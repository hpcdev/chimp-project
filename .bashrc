
if [ -f $HOME/.bashrc ]; then
    . $HOME/.bashrc
fi

source $(dirname $BASH_SOURCE)/.git-completion.bash
