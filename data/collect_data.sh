#!/bin/bash

SCRIPTDIR=$(dirname "$(readlink -f "$0")")

(
    cd "$SCRIPTDIR"/traces && \
    find . -name "*.exp" | \
        while read line
        do
            filename="$(basename $line)"
            partsstr="$(echo "$filename" | sed -n 's/^out-\(.*\)-\([0-9]*\)-\([0-9]*\)-\([0-9]*\)-\([0-9]*\)-\([0-9]*\)-\(.*\)\.exp$/\2-\3-\4-\5-\6-\7/p')"
            dirname="$( echo "$filename" | sed -n 's/^out-\(.*\)-\([0-9]*\)-\([0-9]*\)-\([0-9]*\)-\([0-9]*\)-\([0-9]*\)-\(.*\)\.exp$/\1/p')"
            if [[ -z "$dirname" || -z "$partsstr" ]]
            then
                echo "File $filename does not match pattern. Ignoring..." >&2
                continue
            fi
            parts=(${partsstr//-/ })
            wh=${parts[0]}
            nn=${parts[1]}
            sa=${parts[2]}
            ta=${parts[3]}
            nd=${parts[4]}
            m=${parts[5]}
            im="$(echo "$dirname" | sed -rn 's/^.*(read_com|rep_read|serializable).*$/\1/p')"
            wl="tpcc"
            if [[ -z "$im" ]]
            then
                wl="$dirname"
            fi
            "$SCRIPTDIR"/exp2json.py -i $line -c \
                '{"workload": "'$wl'",
                 "isolation_mode": "'$im'",
                 "num_cores_per_node": '$(($sa + $ta))',
                 "num_server_agents_per_node": '$sa',
                 "num_nodes": '$nn',
                 "num_warehouses": '$wh',
                 "network_delay": '$nd',
                 "mechanism": "'$m'"
                 }'
        done
) > "$SCRIPTDIR"/data.json
