checkmail -a -f -m | sed -e "/Message msg/d" -e "/Return address/d" \
       	   -e 's/ (via .*)//' -e 's/: not yet sent$//' -e 's/^.*@//' |\
sort | uniq -c | sort -n -r
