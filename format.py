#!/usr/bin/env python3
# see LICENSE for license information.
"""
Format C/C++ source files using clang-format.
This script recursively finds all .c, .h, .cpp, and .cc files
and formats them using clang-format with a consistent style.
"""

import os
import subprocess
import sys
from pathlib import Path

# Extensions to format
EXTENSIONS = {'.c', '.h', '.cpp', '.cc'}

def find_source_files(root_dir='.'):
    """Find all C/C++ source files in the directory tree."""
    source_files = []
    for ext in EXTENSIONS:
        # Use glob to find all files with the given extension
        source_files.extend(Path(root_dir).rglob(f'*{ext}'))
    return sorted(source_files)

def format_file(filepath):
    """Format a single file using clang-format."""
    cmd = [
        'clang-format',
        '-i',
        '--style=file',
        '--fail-on-incomplete-format',
        str(filepath)
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f'✓ Formatted: {filepath}')
            return True
        else:
            print(f'✗ Failed to format: {filepath}')
            if result.stderr:
                print(f'  Error: {result.stderr}')
            return False
    except FileNotFoundError:
        print(f'✗ clang-format not found. Please install clang-format.')
        return False
    except Exception as e:
        print(f'✗ Error formatting {filepath}: {e}')
        return False

def main():
    """Main function to format all source files."""
    print('Searching for C/C++ source files...')
    source_files = find_source_files()
    
    if not source_files:
        print('No C/C++ source files found.')
        return 0
    
    print(f'Found {len(source_files)} file(s) to format:\n')
    
    failed_count = 0
    for filepath in source_files:
        if not format_file(filepath):
            failed_count += 1
    
    print(f'\n--- Summary ---')
    print(f'Total files: {len(source_files)}')
    print(f'Successfully formatted: {len(source_files) - failed_count}')
    print(f'Failed: {failed_count}')
    
    return 1 if failed_count > 0 else 0

if __name__ == '__main__':
    sys.exit(main())
