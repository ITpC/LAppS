#!/usr/bin/env bash
#
# Copyright (c) 2007-2009, Pavel Kraynyukhov
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#    * Neither the name of the owner nor the names of its contributors may be
#       used to endorse or promote products derived from this software without
#       specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# $Id: common.sh 200 2010-04-28 11:08:22Z hv11h08 $
################################################################################


PROGNAME=$0

export TR=""

export ECHO="/usr/bin/echo"


OS=$(uname -s)

if [ "$OS" =  "Linux" ] 
then
    TR=/usr/bin/tr
    ECHO=/bin/echo
    AWK=/usr/bin/awk
fi
if [ "$OS" == "SunOS" ]
then
    TR=/usr/xpg4/bin/tr
    ##  use bash internal echo
    ECHO=echo
    AWK=/usr/bin/nawk
fi

if [ "$OS" = "CYGWIN_NT-5.1" ]
then
    TR=/usr/bin/tr
    ECHO=/usr/bin/echo
    AWK=/usr/bin/awk
fi

#ARGS [ value ]
isempty()
{
    if [ "${1}x" == "x" ]
    then
        return 0
    else
        return 1
    fi
}

isoslinux()
{
    if [ "${OS}x" == "Linuxx" ]
    then
        return 0
    else
        return 1
    fi
}

# ARG: STRING
tolower()
{
    $ECHO "$@" | $TR "A-Z" "a-z"
}

toupper()
{
    $ECHO "$@" | $TR "a-z" "A-Z"
}

# Autoclean created files and directories
CLEANUP=""

# ARGS: FILENAME or DIRNAME
cleanup_push()
{
    if [ "${1}x" != "x" ]
    then
        mkdir -p $(dirname /tmp/$1)
        CLEANUP="${CLEANUP} /tmp/$1"
    fi
}

# NOTE: In case the CLEANUP variable is undefined, this function does nothing.
# ARGS: NONE
cleanup()
{
    [ "${CLEANUP}x" == "x" ] && return 0
    for i in $CLEANUP
    do
        trace rm -rf $i
    done
}

debug()
{
    [ "${DEBUG}x" == "x" ] || $ECHO -e "\033[01;34mDEBUG:\033[00m $@"
}

#ARGS: null or more, traceout and execute
trace()
{
    if [ "${TRACE}x" = "x" ] 
    then
        $@ 
        return $?
    else
        $ECHO "TRACE: $@"
        $@ 
        ret=$?
        $ECHO "Returned: $ret"
        return $ret
    fi
}

#ARGS: null or more 
einfo()
{
    $ECHO -e "\033[01;32mI:\033[00m $@"
}

#ARGS: null or more 
eask()
{
    $ECHO -n -e "\033[01;32mQ:\033[00m $@: "
}

#ARGS null or more

say()
{
    $ECHO $@
}

#ARGS VARNAME any
ask_v()
{
    VAR=$1
    shift
    say -n "$@ "
    read ret
    export $(echo ${VAR})="${ret}"
}

#ARGS any
ask()
{
    say -n $@
    read ret
    echo "$ret"
}

#ARGS string
strlen()
{
    echo -n $@ | wc -c
}

#ARGS: double columns print - $1 - padding length, $2 - col 1 , $3 col 2
dc_print()
{
    printf "%-${1}s% ${1}s\n" "$2" "$3"
}

#ARGS: at least 2 arguments: 1 - error number, 2-nd to N-th are the error description
eerror()
{
    ret=$1
    shift
    $ECHO -e "\033[01;31mERROR:\033[00m $@"
    return $ret
}

#ARGS: null or more arguments
abort()
{
    eerror "$@"
    ret=$?
    cleanup
    exit $ret
}

#ARGS: One or more arguments - die reason description
die()
{
    [ "${1}x" == "x" ]  && eerror  255 "$PROGNAME : died without reason description" || eerror 255 "${PROGNAME}: $@"
    cleanup
    exit 255
}

#DESC soft abort - info output and gracefull shutdown
#ARG any string
sabort()
{
    einfo "$@"
    cleanup
    exit 1
}

# ARGS: NONE
smb_exec_usage()
{
    $ECHO "Usage: smb_exec CMD_FILE Username CONN_BASE"
    $ECHO "       CMD_FILE - command file with samba client syntax"
    $ECHO "       Username - BKU Username without domain"
    $ECHO "       CONN_BASE - Connection Base. For example \\sample_serv\location"
}


# ARGS: SERVER CMD USERNAME CONN_BASE
smb_exec()
{
    [ $# -ne 4 ] && ( smb_exec_usage && die "in call of smb_exec: wrong number of arguments" )
    touch smb.conf
    trace smbclient -d 0 -s ./smb.conf  -U "$1\\$3" "$4" "$PASSWORD"< $2
    rm smb.conf
}
######################################### CHECK ARGUMENTS ########################################
#Mandatory arguments with options
MANDATORY=""

#Mandatory arguments without options
MADATORY_SINGLE=""

PARALLEL_MANDATORY=""             ## Parrallel list of mandatories, to the options could be divided to two branches.
                                  ## if the element from this list initialized by cli, then those of MANDATORY list become SOFT
                                  ## example: MANDATORY="test" and the PARALLEL_MANDATORY="execute" ==> " --test this | --execute this " 
                                  ## so one of those must be in cli, however if execute is appears to be in cli test will be not required anymore.
                                  ## test could still stay on cli. would it be ignored or not depends on MANDATORY_IGNORE_FOR_PARALLEL list.
MANDATORY_IGNORE_FOR_PARALLEL=""
MANDATORY_EXPECT_FOR_PARALLEL=""  ## adds mandatory cli options for PARALLEL_MANDATORY list // finish of cli branching. 

#Optional arguments
SOFT=""

#Optional arguments without option
SOFT_SINGLE=""

#ARG: VAR_NAME searchlist
is_in_var()
{
    VAR_NAME=$1
    shift
    for i in "$@"
    do
        for j in $(echo \$$VAR_NAME)
        do
            if [ $i == $j ]
            then
                return 0
            fi
        done
    done
    return 1
}

#ARG: VAR_NAME searchlist
#RETURN STRING
exclude_from_var()
{
    VAR_NAME=$1
    shift

    local NEW_VAR_VAL=""

    for i in "$@"
    do
        export $VAR_NAME="`eval echo  $( echo $VAR_NAME  | awk '{ print \"$\" $0 }' )| sed -e \"s/$i//g\"`"
    done
    eval echo  $( echo $VAR_NAME  | awk '{ print "$" $0 }' )
}

# " << vim syntax highlatning bugfix
# ARGS:  all command line args
pre_check_args() 
{
    local lARG=""
    local ignores="false"
    
    for i in "$@"
    do
        if (echo $i | grep "^\-\-" >/dev/null) 
        then
                lARG=""
                opt=$(echo $i | sed -e 's/--//g'|tr "[a-z]" "[A-Z]")
                if ( echo $PARALLEL_MANDATORY  | grep  "${opt}"  > /dev/null )
                then
                    NM=$(exclude_from_var MANDATORY "${opt}")
                    if [ "${NM}"  != "$MANDATORY" ]
                    then
                        export MANDATORY="$NM"
                        export SOFT="${SOFT} $opt"
                    fi

                    NM=$(exclude_from_var MANDATORY_SYNGLE "${opt}")
                    if [ "${NM}"  != "${MANDATORY_SINGLE}" ]
                    then
                        export MANDATORY_SINGLE="$NM"
                        export SOFT_SINGLE="${SOFT_SINGLE} ${opt}"
                    fi
                    ignores="true"
                fi
                $( echo "export  ${opt}=\"\"" )
                lARG="${opt}"
        else
            if [ "${lARG}x" == "x" ]
            then
                einfo string $i is not associated with any cli option and will be ignored
            else
                $(echo "export ${lARG}=$i")
                lARG=""
            fi
        fi
    done

    if [ "${ignores}x" == "truex" ]
    then
        export MANDATORY=$(exclude_from_var MANDATORY $MANDATORY_IGNORE_FOR_PARALLEL)
        export MANDATORY="${MANDATORY} ${MANDATORY_EXPECT_FOR_PARALLEL}"
    fi
}

# ARGS:  list
create_chk()
{
$ECHO "while [ 1 ]"
$ECHO "do"
$ECHO "    ARG=\"\$1\""
$ECHO "    if [ \"\${ARG}x\" == "x" ]"
$ECHO "    then"
$ECHO "        break"
$ECHO "    fi"
$ECHO "    case \"\$ARG\" in"
for i in $MANDATORY $SOFT
do
    j=$(tolower $i)
    arg="--$j"
    $ECHO -e  "        $arg)\n\t\tshift || break\n\t\t$i=\$1\n\t\t;;"
done
for i in $PARALLEL_MANDATORY $MANDATORY_SINGLE $SOFT_SINGLE
do
    j=$(tolower $i)
    arg="--$j"
    $ECHO -e  "        $arg)\n\t\t$i=Y\n\t\t;;"
done
$ECHO "        --help | --usage)"
$ECHO "            usage && exit 1"
$ECHO "        ;;"
$ECHO "        *)"
$ECHO "            shift || break"
$ECHO "            echo \"Unknown argument: \$ARG\""
$ECHO "        ;;"
$ECHO "    esac"
$ECHO "    shift || break"
$ECHO "done"
}

# ARGS: NONE
check_set()
{
    FAIL=0
    ERR_FILE=/tmp/${PROGNAME}_${USER}.err
    cleanup_push $ERR_FILE
    :> $ERR_FILE
    for i in $MANDATORY $MADATORY_SINGLE
    do
        param=\$$i
        val=$(eval $ECHO $param)
        if [ "${val}" == "x" ]
        then
            eerror 254 "$i is not set" >> $ERR_FILE
            FAIL=1
        else
            debug "$i : $val"
        fi
    done

    if [ $FAIL -eq 1 ]
    then
        usage && cat $ERR_FILE
        $ECHO
        $ECHO
        $ECHO
        HELP=1
        cli_help
        die "Not all required arguments are set !"
    fi
}

# ARGS: "$@"
chk_args()
{
    pre_check_args "$@"
    init_arguments
    CHK_FILE=/tmp/${PROGNAME}_${USER}.chk
    cleanup_push $CHK_FILE
    create_chk > $CHK_FILE
    source $CHK_FILE

    check_set
    return 0
}

#################### INITIALIZE VARIABLES #################################
init_arguments()
{
    for i in $MANDATORY
    do
        export $i=x
    done

    for i in $SOFT
    do
        export $i=""
    done

    for i in $SOFT_SINGLE $MANDATORY_SINGLE
    do
        export $i=""
    done
}
###########################################################################

#ARGS VAR_NAME MESSAGE
gets()
{
    [ $# -lt 2 ] && die "gets() usage: gets VAR_NAME 'Message to send'"
    isoslinux && eask "$2" || say -n "$2"
    read $1
    echo
    echo
}


#ARGS VAR_NAME MESSAGE
getsb()
{
    [ $# -lt 2 ] && die "gets() usage: gets VAR_NAME 'Message to send'"
    isoslinux && eask "$2" || say -n "$2"
    stty -echo
    read $1
    stty echo
    echo
    echo
}

#ARGS VAR_NAME [default value] MESSAGE
getsd()
{
    [ $# -lt 2 ] && die "gets() usage: gets VAR_NAME [default value] 'Message to send'"
    var_name=$1
    if [ $# -ge 3 ]
    then
        local def_val=$2
        export $var_name=$def_val
        shift;shift
        echo -n "$@ [$def_val]: "
        read $var_name
        got_val=$(eval echo \$$var_name)
        if [ "${got_val}x" == "x" ]
        then
            export $var_name=$def_val
        fi
    else  # assume 2 args
        shift
        got_val=$(eval echo \$$var_name)
        while [ "${got_val}x" == "x" ]
        do
            echo -n "$@ []:"
            read $var_name
            got_val=$(eval echo \$$var_name)
        done
    fi
}


#ARGS VAR_NAME VAR_NAME_ENUMS
validate()
{
    for i in $(eval echo \$$2)
    do
        if [ "$i" == "$(eval echo $1)" ]
        then
            return 0
        fi
    done
    return 1
}

#ARGS VARNAME 
print_ex()
{
    VAL=$(eval echo \$$1)
    if [ "${VAL}x" != "x" ]
    then
        echo $1 " : " $VAL
        return 0
    fi
    return 1
}

#ARGS HEX_VALUE
hex_valid()
{
    HEX=$(toupper $1)
    RET=$(toupper $(printf "%.2x" $(echo "ibase=16;$HEX"  | bc) | tr "a-z" "A-Z"))
    if [ "${RET}x" != "${HEX}x" ]
    then
        return 1
    else
        return 0
    fi
}

#ARGS [ MSG ]*
ask_yn()
{
    echo -n "$@ (y/n)[n]? "
    read yn
    if [ "$(toupper $yn)x" == "$(toupper y)x" ]
    then
        return 0
    else
        return 1
    fi
}



#ARGS VAL
istrue()
{
    if [ "${1}x" == "truex" ]
    then
        return 0
    else
        return 1
    fi
}

#ARGS VAR
var2val()
{
    if ! isempty $1
    then
        say $(eval say \$$1)
    else
        say
        abort  255 "Programmers error in var2val()"
    fi
}

#ARGS value
isdecimal()
{
    if ! isempty $1
    then
        return $(echo $1 | egrep "^[0-9]+$" > /dev/null 2>&1)
    else
        return 1
    fi
}

# ---------- argpars usage -------------

usage()
{
    say
    say "Usage: $PROGNAME options"
    say
    if ! isempty $MANDATORY
    then
      say "Following mandatory options with required argument are available:"
      for i in $MANDATORY
      do
          say "  --$(tolower $i) value "
      done
    fi
    say
    if ! isempty $SOFT
    then
      say "Following non mandatory options with required argument are available:"
      for i in $SOFT
      do
          say "[ --$(tolower $i) value ]"
      done
    fi
    say

    if ! isempty $SOFT_SINGLE
    then
      say "Following optional arguments are available:"
      for i in $SOFT_SINGLE
      do
          say "[ --$(tolower $i) ]"
      done
    fi
    exit 0
}

cli_help()
{
    if istrue $HELP
    then
        usage
    fi
}

