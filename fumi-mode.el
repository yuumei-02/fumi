(defvar fumi-mode-keywords 
   '("procedure" "begin" "end" "with" "returns" "return" "while" "do" "if" "else" "then" "break" "continue")
   "Fumi keywords")

(defvar fumi-mode-types 
   '("i8" "i16" "i32" "i64" "isize" "u8" "u16" "u32" "u64" "usize")
   "Fumi build-in types")

(defvar fumi-mode-comments
   '("//.*")
   "Fumi comments")

(defvar fumi-mode-strings
   '("\"\\([^\"\\]\\|\\\\.\\)*\"")
   "Fumi strings")

(defvar fumi-mode-int-literals
   '("\\b[0-9]+\\b")
   "Fumi int literals")

(defvar fumi-mode-procedure-calls
   '("\\<\\([a-zA-Z_][a-zA-Z0-9_]*\\)\\s-*(")
   "Fumi procedure calls")

(defvar fumi-mode-font-lock-keywords
   (let ((kw-regex (regexp-opt fumi-mode-keywords 'words))
        (type-regex (regexp-opt fumi-mode-types 'words)))
   `(
      (,type-regex . font-lock-type-face)
      (,kw-regex   . font-lock-keyword-face)
      (,(car fumi-mode-comments) . font-lock-comment-face)
      (,(car fumi-mode-strings)   . font-lock-string-face)
      (,(car fumi-mode-int-literals) . font-lock-constant-face)
      (,(car fumi-mode-procedure-calls) 1 font-lock-function-name-face)
      ))
   "Font lock for fumi highlights")

(define-derived-mode fumi-mode prog-mode "Fumi"
   "Major mode for editing Fumi files"
   (setq font-lock-defaults '(fumi-mode-font-lock-keywords)))

(add-to-list 'auto-mode-alist '("\\.fumi$" . fumi-mode))
