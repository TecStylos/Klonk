#!/bin/bash

export SPOTIPY_CLIENT_ID=$(cat data/client_id.txt)
export SPOTIPY_CLIENT_SECRET=$(cat data/client_secret.txt)
export SPOTIPY_REDIRECT_URI="http://example.com"

python spotify.py
