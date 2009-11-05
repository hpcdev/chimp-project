
if [ -f $HOME/.bashrc ]; then
    . $HOME/.bashrc
fi


# set a fancy prompt (non-color, unless we know we "want" color)
case "$TERM" in
    xterm-color|xterm|linux) color_prompt=yes;;
esac

# some stuff taken from the debian bashrc for detecting color support
# uncomment for a colored prompt, if the terminal has the capability; turned
# off by default to not distract the user: the focus in a terminal window
# should be on the output of commands, not on the prompt
#force_color_prompt=yes

if [ -n "$force_color_prompt" ]; then
    if [ -x /usr/bin/tput ] && tput setaf 1 >&/dev/null; then
	# We have color support; assume it's compliant with Ecma-48
	# (ISO/IEC-6429). (Lack of such support is extremely rare, and such
	# a case would tend to support setf rather than setaf.)
	color_prompt=yes
    else
	color_prompt=
    fi
fi


# http://henrik.nyh.se/2008/12/git-dirty-prompt
# http://www.simplisticcomplexity.com/2008/03/13/show-your-git-branch-name-in-your-prompt/
# username@Machine ~/dev/dir[master]$ # clean working directory
# username@Machine ~/dev/dir[master*]$ # dirty working directory 
function parse_git_dirty {
    [[ $(git status 2> /dev/null | tail -n1) != \
       "nothing to commit (working directory clean)" ]] && echo "*"
}
function parse_git_branch {
    git branch --no-color 2> /dev/null | \
        sed -e '/^[^*]/d' -e "s/* \(.*\)/[\1$(parse_git_dirty)]/"
}
export -f parse_git_dirty
export -f parse_git_branch

if [ "$color_prompt" = yes ]; then
    export PS1='\u@\h:\[\033[1;33m\]\w\[\033[0m\]$(parse_git_branch)$ '
else
    export PS1='\u@\h:\w$(parse_git_branch)$ '
fi

function gstat {
    (git status ||:) && git submodule foreach 'git status || : '
}
export -f gstat
