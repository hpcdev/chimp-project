#!/bin/bash

DEF_install="$(pwd)/install"
DEF_build="$(pwd)/BuildArea"
DEF_variant="release"

components="boost chimp olson-tools physical"
component_args_boost="--with-regex"

function usage() {
    echo "$0 [options]"
    echo "    The options can be any bjam option including:"
    echo "      --prefix=/path/to/install/into     Default:  ${DEF_install}"
    echo "          Be warned that this must be an ABSOLUTE path"
    echo "      --libdir=/path/to/install/lib      Default:  ${DEF_install}/lib"
    echo "          Be warned that this must be an ABSOLUTE path"
    echo "      --includedir=/path/to/install/inc  Default:  ${DEF_install}/include"
    echo "          Be warned that this must be an ABSOLUTE path"
    echo "      --build-dir=/path/for/building     Default:  ${DEF_build}"
    echo "          Be warned that this must be an ABSOLUTE path"
    echo "      variant=[debug,release,profile]    Default:  ${DEF_variant}"
    echo ""
    echo "        you can also just specify [debug|release|profile] instead of"
    echo "        using the 'variant=' prefix.  "
    echo ""
    echo "Currently, the following pieces will be compiled and installed:"
    printf "component\t\tcomponent specific arguments\n"
    printf "*********\t\t******************\n"
    for i in $components; do
        eval "args=\${component_args_${i//-/_}}"
        printf "  %s\t\t%s\n" $i $args
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
    prefix="--prefix=${DEF_install}"
fi

if [ ! "$(arg_set '--build-dir')" ]; then
    build_dir="--build-dir=${DEF_build}"
fi

if [ ! "$(arg_set 'variant=')" -a \
     ! "$(arg_set release)" -a \
     ! "$(arg_set debug)" -a \
     ! "$(arg_set profile)" ]; then
    variant="variant=${DEF_variant}"
fi

# lets first make sure that the Boost.Build system is up and running...
. project-shell --do-not-start-shell

for i in $components; do
    pushd $i > /dev/null
    echo "building sub-component $i..."
    sleep .8
    eval "args=\${component_args_${i//-/_}}"
    bjam ${args} ${build_dir} ${variant} install ${prefix} "$@"
    popd > /dev/null
done

