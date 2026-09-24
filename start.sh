#!/bin/bash

sqlite3 ./data/backup.db .dump > ./data/backup.sql

./vec_server
