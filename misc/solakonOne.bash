#!/usr/bin/env bash
# bash-completion for solakonOne
# Install: sudo cp misc/solakonOne.bash /etc/bash_completion.d/solakonOne

_solakonOne() {
    local cur prev words cword
    _init_completion || return

    # Options
    local opts="--host= --port= --format= --interval= --help --version"

    case "$prev" in
        --host|-h)
            _filedir
            return
            ;;
        --port)
            COMPREPLY=( $(compgen -W "502" -- "$cur") )
            return
            ;;
        --format)
            COMPREPLY=( $(compgen -W "csv json jsonl" -- "$cur") )
            return
            ;;
        --interval)
            COMPREPLY=( $(compgen -W "1 2 5 10 30 60" -- "$cur") )
            return
            ;;
    esac

    case "$cur" in
        --*)
            COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
            return
            ;;
    esac

    # No more completions after positional args
    if [[ ${#COMP_WORDS[@]} -gt $((cword + 1)) ]]; then
        return
    fi
}

complete -F _solakonOne solakonOne
