#!/usr/bin/env bash
#https://stackoverflow.com/questions/1596945/building-osx-app-bundle

mkdir -p csvquery.app/Contents/MacOS/
mkdir -p csvquery.app/Contents/libs

cp ./cql ./csvquery.app/Contents/MacOS/

cat >> ./csvquery.app/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleGetInfoString</key>
  <string>Csvquery</string>
  <key>CFBundleExecutable</key>
  <string>cql</string>
  <key>CFBundleIdentifier</key>
  <string>com.davosaur.www</string>
  <key>CFBundleName</key>
  <string>cql</string>
  <key>CFBundleIconFile</key>
  <string>cql.icns</string>
  <key>CFBundleShortVersionString</key>
  <string>0.01</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>IFMajorVersion</key>
  <integer>0</integer>
  <key>IFMinorVersion</key>
  <integer>1</integer>
</dict>
</plist>
EOF

dylibbundler -od -b -x ./csvquery.app/Contents/MacOS/cql -d ./csvquery.app/Contents/libs
