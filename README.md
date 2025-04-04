# Component idfx

### C++ component library for esp-idf projects.
The purpose of this library is to make it easier to use C++ to create an esp-idf project for the ESP32 microprocessor.
The prime motivation was to make it easier to manage the Modulencer project, but also to share what I learned about using esp-idf.

### Building out to IDF repository
So that projects can access the idfx library need to build it out to the Espressif component repository. After making
changes to this repo one needs to take the following steps to make them available.

Steps for pushing to repo:
 * If haven't done so yet, login to the repo. Instructions for logging in are at  https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/config_file.html#login-via-cli  . You need access token in order to do this. In VSCode terminal, to login do `compote registry login --profile "default" --registry-url "https://components.espressif.com" --default-namespace skibu`
 * Update idf_component.yml file with new version number
 * in VSCode terminal cd to the directory where have copy of the component (like ~/git/idfx), and then do: `compote component upload --name idfx`

Steps for pulling from repo into VSCode project: 
 * In the VSCode project where you want to use the new version of the library update the idf_component.yml file so that it refers to the new version of the idfx library that was just created.
 * In VSCode terminal execute `idf.py update-dependencies` to load in new version of the library

