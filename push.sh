#!/bin/bash
msg="Uploading files $(date)"
make clean
git add *
git commit -m "$msg"
git push