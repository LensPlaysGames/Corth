;;; corth-mode.el --- Major Mode for editing Corth source code -*- lexical-binding: t -*-

;; Copyright (C) 2022 Rylan Kellogg <lensisme@gmail.com>

;; Author: Rylan Kellogg <lensisme@gmail.com>
;; URL: https://github.com/LensPlaysGames/Corth

;; Permission is hereby granted, free of charge, to any person
;; obtaining a copy of this software and associated documentation
;; files (the "Software"), to deal in the Software without
;; restriction, including without limitation the rights to use, copy,
;; modify, merge, publish, distribute, sublicense, and/or sell copies
;; of the Software, and to permit persons to whom the Software is
;; furnished to do so, subject to the following conditions:

;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.

;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
;; BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
;; ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
;; CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.

;;; Commentary:
;;
;; Major Mode for editing Corth source code. It's Porth but written in C++.

(defconst corth-mode-syntax-table
  (with-syntax-table (copy-syntax-table)
    ;; C/C++ style comments
	(modify-syntax-entry ?/ ". 124b")
	(modify-syntax-entry ?* ". 23")
	(modify-syntax-entry ?\n "> b")
    ;; Chars are the same as strings
    (modify-syntax-entry ?' "\"")
    (syntax-table))
  "Syntax table for `corth-mode'.")

(eval-and-compile
  (defconst corth-keywords '("if" "else" "endif" "do" "while" "endwhile"
							 "dup" "twodup"  "drop" "swap" "over" "mem"
							 "loadb" "loadw" "loadd" "loadq"
							 "storeb" "storew" "stored" "storeq"
							 "dump" "dump_c" "dump_s"
							 "shl" "shr" "or" "and" "mod"
							 "write" "append" "open_file" "write_to_file" "close_file"
							 "length_s")))

(defconst corth-highlights
  `((,(regexp-opt corth-keywords 'symbols) . font-lock-keyword-face)))

;;;###autoload
(define-derived-mode corth-mode prog-mode "corth"
  "Major Mode for editing Corth source code."
  (setq font-lock-defaults '(corth-highlights))
  (set-syntax-table corth-mode-syntax-table))

;; Automatically load mode for any file with the `.corth` extension
;;;###autoload
(add-to-list 'auto-mode-alist '("\\.corth\\'" . corth-mode))
(provide 'corth-mode)

;;; corth-mode.el ends here
