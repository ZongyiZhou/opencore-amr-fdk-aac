if (!(Test-Path .\Sample01_4.wav)) {
	Invoke-WebRequest https://media.xiph.org/audio/HA_2011/Sample01_4.wav -o .\Sample01_4.wav
}

if ((Get-FileHash .\Sample01_4.wav -Algorithm MD5).hash.ToLower() -ne "a5c105544c64ce92c6c5c06d280e6b9c") {
	echo Incorrect checksum for Sample01_4.wav
	exit(1)
}

.\test-encode-decode .\Sample01_4.wav
