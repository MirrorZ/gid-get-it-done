# Ultimate Todo Man
# author: ashmew2
#
# To use from bash:
#  $ . gid.sh

# Automate loading of gid
# This REDIRECTION BELOW is >> (APPEND).
#
# echo "source ${HOME}/gid/gid.sh" >> ~/.bashrc

# Usage:
# tda <list of stuff>

# Example:
# $ tda Get grocery
# $ tda Code a game
# $ tda work hard
#
# $ cat todo.gid
# 0. Get grocery
# 1. Code a game
# 2. work hard

GID_TODO_FILE="${HOME}/mytodo.gid"
BACKUP_FLAG=1
BACKUP_PREFIX=""
BACKUP_SUFFIX=".bck"
REVIEW_THRESHOLD=20

# Function for reviewing the todo list
function tdre {

    # Be safe (not pregnant)
    cp -a ${GID_TODO_FILE} ${GID_TODO_FILE}.$(date +%s)

    IMP_FILE=${GID_TODO_FILE}.i.tmp
    OTHER_FILE=${GID_TODO_FILE}.o.tmp
    NEW_FILE=${GID_TODO_FILE}.new.tmp

    touch $IMP_FILE
    touch $OTHER_FILE
    touch $NEW_FILE

    # rm -f the old files as well. It has a .tmp extension, we expect the user to understand.
    IFS=$'\t\n'; todo_file=( $(cat $GID_TODO_FILE) ); unset IFS

    for item in "${todo_file[@]}"; do

	echo $item

	impbool=y
	read -p "Important? [ [yY]: Yes; [dD]: Delete; [Any other key]: No ]: " impbool

	if [[ "$impbool" == "y" || "$impbool" == "Y" ]]; then
	    echo "$item" >> $IMP_FILE
	elif [[ "$impbool" != "d" && "$impbool" != "D" ]]; then
	    echo "$item" >> $OTHER_FILE
	fi
    done
    cat $IMP_FILE $OTHER_FILE >> $GID_TODO_FILE.new.tmp

    # Show the diff so that user can undo major screw ups if [requ|des]ired
    diff -u $GID_TODO_FILE $NEW_FILE
    > $GID_TODO_FILE
    cat $NEW_FILE > $GID_TODO_FILE

    # Wind up
    rm -f $IMP_FILE
    rm -f $OTHER_FILE
    rm -f $NEW_FILE

    # TODO: Delete the backup here? Probably leave it around for $AGE..
}

function tda {
    # Handle args

    [[ $1 = --file ]] && {
	echo $GID_TODO_FILE
	return 0
    }

    [[ $1 = --show ]] && {
	num=0
	while read -r; do
	    echo -e "====\n#${num}:\n===="
	    echo $REPLY
	    num=$(expr $num + 1)
	done < $GID_TODO_FILE | less
	return 0
    }

    [[ $1 = --usage ]] && {
	echo -e "\tExample usage:"
	echo -e "\t--------------"
	echo -e "\t\t$ tda Go to the grocery store at 13:00"
	echo -e "\t\t$ tda Book a table for my date."
	echo -e "\t\t$ tda fix memory corruption in the network stack"
	echo -e "\t\t$ tda Conquer the world!"
	echo -e "\t--------------"
	echo -e "\ttda --show  : Show your todo list"
	echo -e "\ttda --usage : Print this message and exit"
	echo -e "\ttda --file  : Print the full path to todo file"
	echo -e "\ttdr         : Reload the gid.sh script"
	echo -e "\ttdre        : Review and sort your todo list"
	echo -e "\t--------------"

	return 0
    }
    
    echo "$@" >> $GID_TODO_FILE

    # Honor the REVIEW_THRESHOLD
    num_items=$(wc -l $GID_TODO_FILE | awk '{print $1}')

    if [[ $num_items -ge $REVIEW_THRESHOLD ]]; then
	echo "[Warning] $num_items in TODO. Advice: tdre"
    fi

    if [ $BACKUP_FLAG -eq 1 ]; then
	echo "$@" >> ${BACKUP_PREFIX}${GID_TODO_FILE}${BACKUP_SUFFIX}
    fi
}

#TODO - fix handling of special symbols in input
#TODO - Decide if we need a tda-clean function as well to clean up old files etc or not.
#TODO - Add a 'd' option to tdre for deleting during review
#TODO - Add network sync to keep safe with buffering.
#TODO - Fix the naming scheme of all binaries, maybe tdreload is better than tdr as we can have tdremove and tdedit as well in that case
