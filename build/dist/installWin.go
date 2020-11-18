package main

import (
	"archive/zip"
	"bytes"
	"encoding/json"
	"fmt"
	"time"
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
var infoUrl string = baseurl + `?d=install`
var installPath string = `C:\Program Files\Csvquery`
var programPath string = installPath + `\csvquery\csvquery.exe`
var desktopPath string = os.Getenv(`USERPROFILE`)+`\Desktop\csvquery`
var versionInfo info

type info struct {
	Version      string `json:"version"`
	InstallNotes string `json:"notes"`
}

func main() {
	fmt.Println(title)
	getVersionInfo()
	if needInstall() {
		fmt.Println(`Press Enter to install or update csvquery`)
		prompt()
		install()
	}
	makeShortcut()
	fmt.Printf("\n%s\n",versionInfo.InstallNotes)
	fmt.Println(`Installation/update Complete. You may close this window.`)
	time.Sleep(time.Minute)
}

func prompt() bool {
	var input string
	fmt.Scanln(&input)
	return strings.TrimSpace(strings.ToLower(input)) == `y`
}

func makeShortcut(){
	if _,err := os.Stat(desktopPath); err == nil {
		fmt.Println("Desktop shortcut exists")
		return
	}
	fmt.Printf("\nCreate desktop shortcut? Enter 'y' if yes\n")
	if prompt() == false {
		return
	}
	cmd := exec.Command(`cmd`, `/c`, `mklink`, desktopPath, programPath)
	out, err := cmd.CombinedOutput()
	fmt.Println(string(out))
	if err != nil {
		fmt.Println(err)
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

func getVersionInfo() {
	infobytes, err := downloadBytes(infoUrl)
	if err != nil {
		log.Fatalf("Failed to find out the newest version: %v", err)
	}
	chk(json.Unmarshal(infobytes, &versionInfo))
	versionInfo.Version = strings.TrimSpace(versionInfo.Version)
}

func needInstall() bool {
	//check local version
	cmd := exec.Command(programPath, `version`)
	out, err := cmd.CombinedOutput()
	if err != nil {
		return true
	}
	current := strings.TrimSpace(string(out))
	fmt.Println("Current version:", current)

	//check newest version
	fmt.Println("Newest version:", versionInfo.Version)
	if versionInfo.Version == current {
		fmt.Println("Your version of csvquery is up to date.")
		return false
	}
	return true
}

func install() {
	fmt.Println(installing)
	zipped, err := downloadBytes(fmt.Sprintf(zipurlfmt, versionInfo.Version))
	if err != nil {
		log.Fatalf("Failed to download program: %v", err)
	}
	created, err := extract(zipped)
	chk(err)
	fmt.Println("Created files:")
	for _, f := range created {
		fmt.Println(f)
	}
}

func uninstall() {
	os.RemoveAll(installPath)
	os.RemoveAll(desktopPath)
	fmt.Println(`Uninstalled`)
	time.Sleep(time.Minute)
	os.Exit(0)
}

func chk(err error) {
	if err != nil {
		log.Println(err)
		time.Sleep(time.Minute)
	}
}

func extract(zipped []byte) ([]string, error) {
	var filenames []string
	brd := bytes.NewReader(zipped)
	r, err := zip.NewReader(brd, int64(len(zipped)))
	chk(err)
	for _, f := range r.File {
		fpath := filepath.Join(installPath, f.Name)
		// Check for ZipSlip. More Info: http://bit.ly/2MsjAWE
		if !strings.HasPrefix(fpath, filepath.Clean(installPath)+string(os.PathSeparator)) {
			return filenames, fmt.Errorf("%s: illegal file path", fpath)
		}
		filenames = append(filenames, fpath)
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
