#/bin/bash
#
if [ "$1" == "conf" ]; then
cat <<EOF
"comp":{"name":"diskspace"},

"test":{"name":"diskspace_low_free_test",
		"polled" : true,
        "interval" : "fast",
        "comp":"diskspace"},

"rule":{"name":"diskspace_low_free_rule",
		"input":"diskspace_low_free_test",
		"comp":"diskspace"},

"instance" : {"object":"diskspace_low_free_test",
              "name":"/opt"},

"instance" : {"object":"diskspace_low_free_rule",
              "name":"/opt"},

"instance" : {"object":"diskspace_low_free_test",
              "name":"/"},

"instance" : {"object":"diskspace_low_free_rule",
              "name":"/"},

"ready":["diskspace_low_free_test"]

EOF
fi
if [ "$1" == "test" ]; then
	if [ "$2" == "diskspace_low_free_test" ]; then
		if [ "$#" == "2" ]; then
			cat <<EOF
"result":{"test":"diskspace_low_free_test",
			"result":"ignore"},
"result":{"test":"diskspace_low_free_test",
			"instance":"/opt",
			"value":12},
"result":{"test":"diskspace_low_free_test",
			"instance":"/",
			"value":6}
EOF
			exit 0
		elif [ "$#" == "3" ]; then
			instance=$3
cat <<EOF
"result":{"test":"diskspace_low_free_test",
			"instance":"$instance",
			"result":"ignore"}
EOF
		fi
		exit 0
    fi
fi

