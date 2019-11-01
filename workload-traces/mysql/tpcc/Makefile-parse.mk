MAKEFILE=$(realpath $(lastword $(MAKEFILE_LIST)))
SCRIPT_DIR=$(shell dirname $(MAKEFILE))

# Input files
TRACEFILE_SERVER=server.trc
TRACEFILE_CLIENT=client.trc

# Derived files
TRACEFILE_COMBINED=combined.trc.xz
WH_TRACEFILES=$(sort $(wildcard $(TMPFOLDER)/wh*.out.xz))
WH_PARSED_TRACEFILES=$(WH_TRACEFILES:%.out.xz=%-parsed.json.xz)
WH_LOCKNAMEFILES=$(WH_TRACEFILES:.out.xz=-locks.json.xz)
WH_LOCKSPLITFILES=$(WH_TRACEFILES:.out.xz=.split-locks)
WH_ALLLOCKFILES=$(WH_TRACEFILES:.out.xz=-alllocks.json.xz)
WH_EVENTFILES=$(WH_TRACEFILES:.out.xz=-events.json.xz)
WH_LOCKIDFILES=$(WH_TRACEFILES:.out.xz=-lockids.json.xz)
WH_LOCKMETAFILES=$(WH_TRACEFILES:.out.xz=-locks-meta.json)
LOCKTRACEFILES=$(patsubst $(TMPFOLDER)/%.out.xz,%.csv.xz,$(WH_TRACEFILES))
TMPLOCKTRACEFILES=$(patsubst $(TMPFOLDER)/%.out.xz,%-events.json.xz,$(WH_TRACEFILES))
LOCKIDS_JSON=lockids.json.xz
# Representative segments file: whether or not the target needs to be rebuilt
# should depend on this file:
SEGMENTS=$(TMPFOLDER)/dummy.out
TMPFOLDER=tmp

# Scripts
SEGMENTIZE=$(SCRIPT_DIR)/segmentize.py
PARSE_SEGMENT=$(SCRIPT_DIR)/parse_segment.py
EXTRACT_LOCK_NAMES=$(SCRIPT_DIR)/extract_lock_names.py
GENERATE_LOCK_IDS=$(SCRIPT_DIR)/generate_lock_ids.py
EXTRACT_EVENTS=$(SCRIPT_DIR)/extract_events.py
SPLIT_LOCKS=$(SCRIPT_DIR)/split_locks_by_warehouse.py
MAKE_TRACE=$(SCRIPT_DIR)/make_trace/build/make_trace

PIXZ?=pixz
SORTFLAGS_BIG=--parallel=16 -S 200G $(SORTFLAGS)
SORTFLAGS_SMALL=-S 20G $(SORTFLAGS)

SHELL=/bin/bash -o pipefail

# Targets

.DELETE_ON_ERROR:
.SECONDARY: $(WH_PARSED_TRACEFILES) $(WH_EVENTFILES) $(WH_ALLLOCKFILES) \
	$(WH_LOCKIDFILES) $(WH_LOCKNAMEFILES) $(WH_LOCKMETAFILES) \
	$(WH_LOCKSPLITFILES)

# The makefile calls itself several times because some rules need to know which
# files have been created by other rules and this list of files cannot be
# predicted. When make calls itself, the inner invocation sees the files
# created by the outer invocation and can execut the rules correctly.

# Outer-most invocation of make (level 0) -------------------------------------

# Main target: trace files in the format of the replay client

# The value of $(LOCKTRACEFILES) depends on the output of $(SEGMENTIZE), so
# we first call that and then call make again.
.PHONY: tracefiles
tracefiles: segments
	$(MAKE) -f $(MAKEFILE) make_tracefiles0

# Merge tracefile of client and server
$(TRACEFILE_COMBINED): $(TRACEFILE_SERVER) $(TRACEFILE_CLIENT)
	cat $^ | LANG=C sort $(SORTFLAGS_BIG) | $(PIXZ) > $(TRACEFILE_COMBINED)

# Segmentize the input trace file, i.e., produce one file per transaction
segments: $(SEGMENTS)
$(SEGMENTS): $(TRACEFILE_COMBINED) | $(TMPFOLDER)
	cd $(TMPFOLDER) &&  $(PIXZ) -d ../$(TRACEFILE_COMBINED) /dev/stdout \
		| sed "s/^[0-9 ]*: //" | $(SEGMENTIZE)

$(TMPFOLDER):
	mkdir -p $@

# Other
.PHONY: clean
clean:
	rm -rf $(SEGMENT_FILES) $(TRACEFILE_COMBINED) $(LOCKTRACEFILES) \
		$(LOCKIDS_JSON) $(TMPFOLDER) max-locks-per-wh.meta

# Nested invocation of make (level 1) -----------------------------------------

# This level extracts all information that comes from the tracefile *of a
# single partition*. In particular does it split the locks from each tracefile
# by partition (client for a particular warehouse may make (remote) access to
# different warehouses).

.PHONY: make_tracefiles0
make_tracefiles0: $(WH_LOCKSPLITFILES)
	$(MAKE) -f $(MAKEFILE) make_tracefiles1

# Parse the tracefile to json
$(TMPFOLDER)/wh%-parsed.json.xz: $(TMPFOLDER)/wh%.out.xz
	cat $^ | xz -fd | $(PARSE_SEGMENT) | xz > $@

# Extract lock events
$(TMPFOLDER)/wh%-events.json.xz: $(TMPFOLDER)/wh%-parsed.json.xz
	cat $< | xz -d | $(EXTRACT_EVENTS) | xz > $@

# Extract lock names
$(TMPFOLDER)/wh%-locks.json.xz: $(TMPFOLDER)/wh%-parsed.json.xz
	cat $^ | xz -d | $(EXTRACT_LOCK_NAMES) | xz > $@

# Split locks by owner warehouse
$(TMPFOLDER)/wh%.split-locks: $(TMPFOLDER)/wh%-locks.json.xz
	xzcat $^ | LANG=C sort $(SORTFLAGS_SMALL) | $(SPLIT_LOCKS) $(@:.split-locks=)

# Nested invocation of make (level 2) -----------------------------------------

make_tracefiles1: $(LOCKTRACEFILES)

# Dynamic rule pattern:
#
# Defines a pattern rule where all of $(INPUTS) are prerequesites for all of
# $(OUTPUTS) through a single invocation of this rule. It must be a pattern
# rule because non-pattern rules cannot generate multiple targets. To achieve
# this, a common suffix in $(INPUTS) is removed and turned into the pattern
# stem (e.g., if $(INPUTS) is 'a.txt b.txt' and $(OUTPUTS) is
# 'out-a.txt out-b.txt', then the generated pattern rules is 'a.% b.%: ...').
# Defining a pattern rule comming from a list is only possible with dynamic
# rules (with eval). Note that the first usage of '%' is replaced with a stem
# and the second usage is left as it is (which is what we need: it is the
# wildcard of the generated pattern rule).
#
# define makerule
# $(patsubst out-%.txt,out-%.$(PERCENT),$(OUTPUTS)): $(patsubst %.txt,%.$(PERCENT),$(INPUTS))
# 	@echo use $$^ to make $$@
# 	touch $$(patsubst tmp/%,out-%,$$^)
# endef

# This dynamic rule creates all %-events.json.xz using all
# tmp/%-events.json.xz using a single invocation of the rule:
$(eval $(call makerule))
define maketracefilerule
$(patsubst %-events.json.xz,%-events.json.%,$(TMPLOCKTRACEFILES)): \
	$(patsubst $(TMPFOLDER)/%-events.json.xz,$(TMPFOLDER)/%-events.json.%,$(WH_EVENTFILES)) $(LOCKIDS_JSON)
	$$(MAKE_TRACE) -l <(xzcat $$(LOCKIDS_JSON)) \
		-i $(patsubst %,<(xzcat %),$(WH_EVENTFILES)) \
		-o $(patsubst %,>(xz > %),$(TMPLOCKTRACEFILES))
endef
$(eval $(call maketracefilerule))
.INTERMEDIATE: $(TMPLOCKTRACEFILES)

# Main output files
#
# The dynamic rule creates its output with a weird name (it uses the same
# suffix as its input in order to be a pattern rule), so we rename the final
# output files:
wh%.csv.xz: wh%-events.json.xz
	mv $^ $@

# Merge locks of each warehouse found in traces of different clients
$(TMPFOLDER)/wh%-alllocks.json.xz: $(TMPFOLDER)/wh%.split-locks
	zcat -f $(wildcard $(TMPFOLDER)/wh*-wh$(patsubst $(TMPFOLDER)/wh%-alllocks.json.xz,%,$@).json.gz) /dev/null \
		| LANG=C sort $(SORTFLAGS_SMALL) -u | xz > $@

# Count the number of locks per warehouse
$(TMPFOLDER)/wh%-locks-meta.json: $(TMPFOLDER)/wh%-alllocks.json.xz
	xzcat $^ | wc -l > $@

# Compute the maximum number of locks per warehouse
max-locks-per-wh.meta: $(WH_LOCKMETAFILES)
	cat $^ | (LANG=C sort $(SORTFLAGS_SMALL) -rn || true) | head -n1 > $@

# Generate lock IDs per warehouse
$(TMPFOLDER)/wh%-lockids.json.xz: $(TMPFOLDER)/wh%-alllocks.json.xz max-locks-per-wh.meta
	MAXLOCKSPERWH=`cat max-locks-per-wh.meta`; \
	WH=$(patsubst $(TMPFOLDER)/wh%-lockids.json.xz,%,$@); \
	OFFSET=$$(echo "($$WH-1)*$$MAXLOCKSPERWH" | bc); \
	xzcat $< | $(GENERATE_LOCK_IDS) $$OFFSET | xz > $@

# Merge lock IDs of all warehouses to a single file
$(LOCKIDS_JSON): $(WH_LOCKIDFILES)
	cat $^ > $@
