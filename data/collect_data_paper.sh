#!/bin/bash

SCRIPTDIR=$(dirname "$(readlink -f "$0")")

rm -f "$SCRIPTDIR"/data.json

(
    cd "$SCRIPTDIR"/traces_paper && \
    find . -name "*.exp" | \
        while read line
        do
            b=$(basename $line | cut -f1 -d. | sed -E "s/--*(hashlock|neworder|nowait|waitdie|timestamp|uni|high|med)/-\\1/g")
            parts=(${b//-/ })
            IFS=$'-'; parts=($b); unset IFS
            has_nd=0; nd='null'
            if [[ ${#parts[@]} -eq 8 ]]; then
                nd=${parts[6]}
                has_nd=1
            fi
            im=${parts[1]}
            wh=${parts[2]}
            if [[ "$wh" == "2520" ]]; then wh=2048; fi
            nn=${parts[3]}
            sa=${parts[4]}
            ta=${parts[4]}
            o1=${parts[$((6+$has_nd))]}
            o2=${parts[$((7+$has_nd))]}
            wl="tpcc"
            if [[ "$o1" == "uni" || "$o1" == "med" || "$o1" == "high" ||
                  "$o2" == "uni" || "$o2" == "med" || "$o2" == "high" ]]
            then
                wl="ycsb"
            fi
            "$SCRIPTDIR"/exp2json.py -i $line -c \
                '{"workload": "'$wl'",
                 "isolation_mode": "'$im'",
                 "num_cores_per_node": '$(($sa + $ta))',
                 "num_server_agents_per_node": '$sa',
                 "num_nodes": '$nn',
                 "num_warehouses": '$wh',
                 "network_delay": '$nd',
                 "option1": "'$o1'",
                 "option2": "'$o2'"
                 }'
        done
) >> "$SCRIPTDIR"/data.json

