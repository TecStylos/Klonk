import os
import struct
import spotipy
from spotipy.oauth2 import SpotifyOAuth
from spotipy.oauth2 import SpotifyClientCredentials

import urllib.request

READ_FILE_DESC = int(os.getenv("PY_READ_FILE_DESC"))
WRITE_FILE_DESC = int(os.getenv("PY_WRITE_FILE_DESC"))
READ_PIPE = os.fdopen(READ_FILE_DESC, "rb", 0)
WRITE_PIPE = os.fdopen(WRITE_FILE_DESC, "wb", 0)

scopes = (
	"user-modify-playback-state",
	"user-read-playback-state",
	"user-read-currently-playing",
	"user-read-recently-played",
	"user-top-read",
	"playlist-read-collaborative",
	"playlist-read-private",
	"user-library-read"
)

def _recvNum(num):
	buff = b''
	while num > 0:
		data = READ_PIPE.read(num)
		if data == '':
			raise RuntimeError("Unexpected EOF")
		buff += data
		num -= len(data)
	return buff

def recvMessage():
	ignoreResponseBuff = _recvNum(1)
	ignoreResponse = struct.unpack("<B", ignoreResponseBuff)[0]
	msgSizeBuff = _recvNum(4)
	msgSize = struct.unpack("<I", msgSizeBuff)[0]
	message = _recvNum(msgSize)
	return ignoreResponse, message.decode("utf-8")

def sendMessage(message):
	#print(message)
	encoded = message.encode("utf-8")
	encodedSize = struct.pack("<I", len(encoded))
	WRITE_PIPE.write(encodedSize)
	WRITE_PIPE.write(encoded)

if __name__ == "__main__":
	spotify = spotipy.Spotify(auth_manager=SpotifyOAuth(scope=" ".join(scopes)))
	result = ""
	while True:
		ignoreResponse, command = recvMessage()
		try:
			exec(f"result = {command}", globals(), locals())
		except KeyboardInterrupt:
			print("KBD INTERRUPT: EXITING")
			break
		except BaseException as err:
			print("EXEC ERROR")
			print(err)
			result = str(err)

		if (not ignoreResponse):
			if not isinstance(result, (list, dict, int, bool, type(None))):
				result = str(result)

			if isinstance(result, str):
				result = result.replace("'", "\"")
				result = f"'{result}'"

			#print(f"---- RESULT ----\n{result}\n----------------")
			sendMessage(str(result))
