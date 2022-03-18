How to Run:
1. Navigate to project folder
2. Run "make"
3. Ensure that file "cstest" is empty for best results
4. Invoke program using "./master # [-t #]"
    - The first '#' is the number of processes you want to run. It will default to 2
if not given. Minimum 1, max 20.
    - The second '#', preceded by '-t' is the maximum amount of time you want the
program to run for. It will default to 100 if not given.

Git Repository: https://github.com/Netsaken/UMSL-CS-4760-Project-3

Problems:
- The processes all print in order. Not sure that's supposed to happen.
- Any issues pertaining to signal handling from P2 were not fixed.
