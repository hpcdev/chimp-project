#!/bin/bash

DEF_install="$(pwd)/install"
DEF_build="$(pwd)/BuildArea"
DEF_variant="release"

components=" boost chimp olson-tools physical "
component_args_boost=" --with-regex link=static "

function usage() {
    echo "$0 [component list] [--] [options] [--help]"
    echo "    [component list]:  The components to build and install"
    echo "      If you don't specify a component to build and install, all"
    echo "      components will be built and installed."
    echo "      NOTE:  for simplicity of this script, the component list MUST"
    echo "      be first in the list of command line arguments to this script"
    echo ""
    echo "      Currently, the following pieces will be compiled and installed:"
    printf "      component\t\tcomponent specific arguments\n"
    printf "      *********\t\t******************\n"
    for i in $components; do
        eval "      args=\${component_args_${i//-/_}}"
        printf "        %s\t\t%s\n" "$i" "$args"
    done
    echo ""
    echo "    The optional '--' separation characters may be necessary if a"
    echo "    component happens to have the same name as a bjam option."
    echo ""
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
    echo "    --help show this message"
}

# create list of componets requested to build
requested_components=""
for i in "$@" ; do
    if [ "${components// $i /}" != "${components}" ]; then
        shift 1
        requested_components=" $requested_components $i "
    else
        break
    fi
done

if [ "$requested_components" != "" ]; then
    components="$requested_components"
fi

#get rid of the possible -- separation character
if [ "$1" = "--" ]; then
    shift 1
fi



all_args="$*"
function arg_set() {
    grep_result=$(echo " ${all_args} " | grep "$1")
    if [ "$grep_result" != "" ]; then
        echo 1
        return 1
    fi
}

if [ "$(arg_set '\(--\)help')" ]; then
    usage
    exit;
fi

if [ ! "$(arg_set '\(--\)prefix')" ]; then
    prefix="--prefix=${DEF_install}"
    install_dir="${DEF_install}"
else
    install_dir=$(echo "${all_args}" | \
        sed -e "s/.*--prefix=\([^[:space:]]*\)\s*.*/\1/")
fi

if [ ! "$(arg_set '\(--\)build-dir')" ]; then
    build_dir="--build-dir=${DEF_build}"
fi

if [ ! "$(arg_set '[[:space:]]\<variant=')" -a \
     ! "$(arg_set '[[:space:]]\<release[[:space:]]\>')" -a \
     ! "$(arg_set '[[:space:]]\<debug\>[[:space:]]')" -a \
     ! "$(arg_set '[[:space:]]\<profile\>[[:space:]]')" ]; then
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

# copy over the build config for Make and CMake
for i in Makefile.chimp-project ; do
    echo "installing $i to ${install_dir}"
    mkdir -p ${install_dir}/share/chimp
    cat "$i" | sed -e "s/@CHIMP_INSTALL_DIR@/${install_dir//\//\\/}/" > \
        ${install_dir}/share/chimp/$i
done
