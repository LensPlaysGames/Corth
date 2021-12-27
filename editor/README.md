# Corth Mode
### A major mode for editing Corth source code.

To use this mode, first place it in somewhere you've instructed emacs to load custom files from. \
After that's done, simply place this in your `.emacs` configuration file: \
`(require 'corth-mode)` \
Now Corth Mode will automatically load in any file with the `.corth` extension.

How to add a custom directory to load paths: \
`(add-to-list 'load-path "~/.emacs.d/custom-directory")`