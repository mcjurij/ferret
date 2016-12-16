# ferret

An easy to use and fast build system for Linux and C/C++ users. Get started by [bootstrapping](https://github.com/mcjurij/ferret/wiki/Getting-started) and see how ferret builds itself.

A project with roughly 2100 C++ compilation units is build in around 4 minutes on a 8 core machine. When the build is up to date, the answer to the question "what do I have to build now?" is given in about 1 second (when the file system cache is hot) for the same amount of files.

Ferret reads its configuration from very simple XML files. One with global options (compiler flags, external libraries etc.) and one for each directory called project.xml. Every executable or library has its own sub directory and project.xml. Using the sub-tag in a project.xml you can refer to another project.xml. Adding a file to the build is simple: You simply put it in the `src` directory that is alongside the project.xml file and it becomes part of the build. 

To get started jump to the wiki [home](https://github.com/mcjurij/ferret/wiki).

After a hard working day, ferret likes to drink a [beer](fursty-ferret.jpeg) ...or two.
