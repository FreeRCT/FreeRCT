#!/usr/bin/env python
# encoding: utf-8

"""Checks whether clang-tidy warnings that were previously cleaned have
regressed."""

import re
import sys

CHECK_REGEX = re.compile(r'.*\[([A-Za-z0-9.-]+)\]$')


def main():
    """Checks whether clang-tidy warnings that were previously cleaned have
    regressed."""
    if len(sys.argv) == 2:
        print('########################################################')
        print('########################################################')
        print('###                                                  ###')
        print('###   Checking clang-tidy results                    ###')
        print('###                                                  ###')
        print('########################################################')
        print('########################################################')
    else:
        print(
            'Usage: check_clang_tidy_results.py <log_file>')
        return 1

    log_file = sys.argv[1]

    errors = 0

    with open(log_file) as checkme:
        contents = checkme.readlines()
        for line in contents:
            if 'third_party' in line:
                continue
            # We're not piloting alpha-level checks
            if 'clang-analyzer-alpha' in line:
                continue
            if CHECK_REGEX.match(line):
                print(line.strip())
                errors = errors + 1

    if errors > 0:
        print('########################################################')
        print('########################################################')
        print('###                                                  ###')
        print('###   Found %s error(s)                               ###'
              % errors)
        print('###                                                  ###')
        print('########################################################')
        print('########################################################')
        print('Information about the checks can be found on:')
        print('https://clang.llvm.org/extra/clang-tidy/checks/list.html')
        return 1

    print('###                                                  ###')
    print('###   Check has passed                               ###')
    print('###                                                  ###')
    print('########################################################')
    print('########################################################')
    return 0


if __name__ == '__main__':
    sys.exit(main())
