#!/bin/bash

declare -A DEF
DEF["install"]="$(pwd)/install"
DEF["build"]="$(pwd)/BuildArea"
DEF["variant"]="release"

components="boost chimp olson-tools physical"
declare -A component_args
component_args["boost"]="--with-regex"

function usage() {
    echo "$0 [options]"
    echo "    The options can be any bjam option including:"
    echo "      --prefix=/path/to/install/into     Default:  ${DEF["install"]}"
    echo "      --build-dir=/path/for/building     Default:  ${DEF["build"]}"
    echo "      variant=[debug,release,profile]    Default:  ${DEF["variant"]}"
    echo ""
    echo "        you can also just specify [debug|release|profile] instead of"
    echo "        using the 'variant=' prefix.  "
    echo ""
    echo "Currently, the following pieces will be compiled and installed:"
    printf "component\t\tcomponent specific arguments\n"
    printf "*********\t\t******************\n"
    for i in $components; do
        printf "  %s\t\t%s\n" $i ${component_args[$i]}
    done
    echo ""
    echo "      --help show this message"
}


all_args="$*"
function arg_set() {
    if [ "${all_args//$1/}" != "${all_args}" ]; then
        echo 1
        return 1
    fi
}

if [ "$(arg_set '--help')" ]; then
    usage
    exit;
fi

if [ ! "$(arg_set '--prefix')" ]; then
    prefix="--prefix=${DEF["install"]}"
fi

if [ ! "$(arg_set '--build-dir')" ]; then
    build_dir="--build-dir=${DEF["build"]}"
fi

if [ ! "$(arg_set 'variant=')" -a \
     ! "$(arg_set release)" -a \
     ! "$(arg_set debug)" -a \
     ! "$(arg_set profile)" ]; then
    variant="variant=${DEF["variant"]}"
fi

# lets first make sure that the Boost.Build system is up and running...
. project-shell --do-not-start-shell

for i in $components; do
    pushd $i > /dev/null
    echo "building sub-component $i..."
    sleep .8
    bjam ${component_args[$i]} ${build_dir} ${variant} install ${prefix} "$@"
    popd > /dev/null
done

