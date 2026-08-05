#!/usr/bin/env bash
# bash-completion for solakon-monitor
# Install: sudo cp misc/solakon-monitor.bash /etc/bash_completion.d/solakon-monitor

_solakon-monitor() {
    local cur prev words cword
    _init_completion || return

    local opts="--help --once --json --server --interval= --port="
    local short_opts="-h -p"

    case "$prev" in
        --interval|-i)
            COMPREPLY=( $(compgen -W "1 2 5 10 30 60" -- "$cur") )
            return
            ;;
        --port|-p)
            COMPREPLY=( $(compgen -W "502" -- "$cur") )
            return
            ;;
        --server)
            # Optional port after --server
            if [[ "$cur" == -* ]]; then
                COMPREPLY=()
            else
                COMPREPLY=( $(compgen -W "8080 9090 3000" -- "$cur") )
            fi
            return
            ;;
    esac

    case "$cur" in
        --*)
            COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
            return
            ;;
        -*)
            COMPREPLY=( $(compgen -W "$short_opts" -- "$cur") )
            return
            ;;
    esac

    # No more completions after positional args
    if [[ ${#COMP_WORDS[@]} -gt $((cword + 1)) ]]; then
        return
    fi
}

complete -F _solakon-monitor solakon-monitor
