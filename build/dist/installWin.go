package main

import (
	"archive/zip"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

var baseurl string = `https://davosaur.com/csv/`
var zipurlfmt string = baseurl + `w/csvquery-win-%s.zip`
var versionurl string = baseurl + `?d=version`
var installPath string = `C:\Program Files\Csvquery`
var programPath string = installPath + `\csvquery\csvquery.exe`
var testing bool = true

type info struct {
	Version string `json:"version"`
}

func main() {
	fmt.Println(title)
	ok, version := shouldInstall()
	if ok {
		install(version, installPath)
	}
}

func downloadBytes(url string) ([]byte, error) {
	client := http.Client{
		CheckRedirect: func(r *http.Request, via []*http.Request) error {
			r.URL.Opaque = r.URL.Path
			return nil
		},
	}
	resp, err := client.Get(url)
	if err != nil {
		return nil, err
	}
	return ioutil.ReadAll(resp.Body)
}

func newestVersion() string {
	infobytes, err := downloadBytes(versionurl)
	if err != nil {
		log.Fatalf("Failed to find out the newest version: %v", err)
	}
	var v info
	chk(json.Unmarshal(infobytes, &v))
	return strings.TrimSpace(v.Version)
}

func shouldInstall() (bool, string) {
	//check local version
	cmd := exec.Command(programPath, `version`)
	out, err := cmd.CombinedOutput()
	if err != nil {
		return true, newestVersion()
	}
	current := strings.TrimSpace(string(out))
	fmt.Println("Current version:", current)

	//check newest version
	newversion := newestVersion()
	fmt.Println("Newest version:", newversion)
	if newversion == current {
		fmt.Println("Your version of csvquery is up to date.")
		return false, ""
	}
	return true, newversion
}

func install(version, destdir string) {
	zipped, err := downloadBytes(fmt.Sprintf(zipurlfmt, version))
	if err != nil {
		log.Fatalf("Failed to download program: %v", err)
	}
	fmt.Println(installing)
	created, err := extract(zipped, destdir)
	chk(err)
	fmt.Println("Created files:")
	for _, f := range created {
		fmt.Println(f)
	}
}

func chk(err error) {
	if err != nil {
		log.Fatalln(err)
	}
}

func extract(zipped []byte, destdir string) ([]string, error) {
	var filenames []string
	brd := bytes.NewReader(zipped)
	r, err := zip.NewReader(brd, int64(len(zipped)))
	chk(err)
	for _, f := range r.File {
		fpath := filepath.Join(destdir, f.Name)
		// Check for ZipSlip. More Info: http://bit.ly/2MsjAWE
		if !strings.HasPrefix(fpath, filepath.Clean(destdir)+string(os.PathSeparator)) {
			return filenames, fmt.Errorf("%s: illegal file path", fpath)
		}
		filenames = append(filenames, fpath)
		if testing {
			continue
		}
		if f.FileInfo().IsDir() {
			// Make Folder
			os.MkdirAll(fpath, os.ModePerm)
			continue
		}
		// Make File
		if err = os.MkdirAll(filepath.Dir(fpath), os.ModePerm); err != nil {
			return filenames, err
		}
		outFile, err := os.OpenFile(fpath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, f.Mode())
		if err != nil {
			return filenames, err
		}
		rc, err := f.Open()
		if err != nil {
			return filenames, err
		}
		_, err = io.Copy(outFile, rc)
		outFile.Close()
		rc.Close()
		if err != nil {
			return filenames, err
		}
	}
	return filenames, nil
}

var title string = `   ____________    ______  __  ______________  __
  / ____/ ___/ |  / / __ \/ / / / ____/ __ \ \/ /
 / /    \__ \| | / / / / / / / / __/ / /_/ /\  / 
/ /___ ___/ /| |/ / /_/ / /_/ / /___/ _, _/ / /  
\____//____/ |___/\___\_\____/_____/_/ |_| /_/   
                                                 `
var installing string = `_ _  _ ____ ___ ____ _    _    _ _  _ ____ 
| |\ | [__   |  |__| |    |    | |\ | | __ 
| | \| ___]  |  |  | |___ |___ | | \| |__] 
                                           
`
