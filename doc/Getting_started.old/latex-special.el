(message "-> Custumized latex.el and hilit-LaTeX.el startup files")

(require 'latex)

(setq LaTeX-indent-environment-list
      (append
       '(("program" current-indentation)
	 ("shell" current-indentation)
	 ("script" current-indentation)
	 ("makefile" current-indentation))
       LaTeX-indent-environment-list))

(require 'hilit-LaTeX)

(setq hilit-auto-highlight-maxout 200000)

(hilit-set-mode-patterns
 '(LaTeX-mode japanese-LaTeX-mode slitex-mode SliTeX-mode japanese-SliTeX-mode 
              FoilTeX-mode latex-mode latex2e-mode ams-latex-mode)
 (append
;;; LB:  '(("\\(^\\|[^\\]\\)\\(%.*\\)$" 2 comment)) ; comments

;;; LB: \|name|
  '(("\\(\\\\|\\)\\([^|][^|]*\\)\\(|\\)" 2 label)) 
;;; End of addition

  (cond 
   (hilit-AmSLaTeX-commands
    '(("\\\\\\(\\(no\\)?pagebreak\\|\\(new\\|clear\\(double\\)?\\)page\\|enlargethispage\\|\\(no\\)?linebreak\\|newline\\|-\\|displaybreak\\|allowdisplaybreaks\\)"
     nil error)
      
      ("\\\\\\(\\(\\(text\\)?\\(rm\\|sf\\|tt\\|bf\\|md\\|it\\|sl\\|sc\\|up\\|em\\|emph\\)\\(series\\|family\\|shape\\)?\\)\\|\\(appendix\\|tableofcontents\\|listoffigures\\|listoftables\\|normalsize\\|small\\|footnotesize\\|scriptsize\\|tiny\\|large\\|Large\\|LARGE\\|huge\\|Huge\\|raggedright\\|makeindex\\|makeglossary\\|pmb\\|boldsymbol\\)\\)\\b" 
     nil decl)

    ;; various declarations/definitions
    ("\\\\\\(maketitle\\|setlength\\|settowidth\\|addtolength\\|setcounter\\|addtocounter\\)\\b" 
     nil define)
    ("\\\\\\([a-z]+box\\|text\\|intertext\\)\\b" nil keyword)))

   (t
    '(("\\\\\\(\\(no\\)?pagebreak\\|\\(new\\|clear\\(double\\)?\\)page\\|enlargethispage\\|\\(no\\)?linebreak\\|newline\\|-\\)"
     nil error)

      ("\\\\\\(\\(\\(text\\)?\\(rm\\|sf\\|tt\\|bf\\|md\\|it\\|sl\\|sc\\|up\\|em\\|emph\\)\\(series\\|family\\|shape\\)?\\)\\|\\(appendix\\|tableofcontents\\|listoffigures\\|listoftables\\|normalsize\\|small\\|footnotesize\\|scriptsize\\|tiny\\|large\\|Large\\|LARGE\\|huge\\|Huge\\|raggedright\\|makeindex\\|makeglossary\\)\\)\\b" 
       nil decl)

      ;; various declarations/definitions
      ("\\\\\\(maketitle\\|setlength\\|settowidth\\|addtolength\\|setcounter\\|addtocounter\\)\\b" 
       nil define)
      ("\\\\[a-z]+box\\b" nil keyword))))


  '(("``" "''" string))
  (and hilit-multilingual-strings
       '(("\"<" "\">" string)
         ("\"`" "\"'" string)
         ("<<" ">>" string)
         ("«" "»" string)))
    
  '(("\\\\\\(item\\(\\[.*\\]\\)?\\|\\\\\\(\*\\)?\\)" nil label) ;label, \\
    ("\\(^\\|[^\\\\]\\)\\(&+\\)" 2 label)     ; & within tables and such
    ;; "wysiwyg" emphasis
    (hilit-bracket-wysiwyg 
     "{\\\\\\(text\\)?\\(em\\|it\\|sl\\)\\(shape\\|family\\|series\\)?\\b" italic)
    (hilit-bracket-wysiwyg              ;Removed rm from list
     "{\\\\\\(text\\)?\\(bf\\|md\\|sc\\|up\\|tt\\|sf\\)\\(shape\\|family\\|series\\)?\\b" 
     bold))
    
  (cond 
   (hilit-AmSLaTeX-commands
    '((hilit-inside-bracket-region         ;also \boldsymbol{<>}, \pmb{<>},
       "\\\\\\(boldsymbol\\|pmb\\|text\\(bf\\|md\\|rm\\|sf\\|tt\\|sc\\|up\\)\\){" 
       bold)

      (hilit-bracket-region 
       "\\\\\\(\\(page\\|v\\|eq\\)?ref\\|tag\\|eqref\\|label\\|index\\|glossary\\|[A-Za-z]*cite[A-Za-z]*\\(\\[.*\\]\\)?\\){" 
       crossref)                          ; added \tag{} \eqref{}
      ("\\\\notag\\b" nil crossref)       ; and \notag

      (hilit-inside-environment
       "\\\\begin{\\(equation\\|eqnarray\\|gather\\|multline\\|align\\|x*alignat\\)\\(\*\\)?}" 
       glob-struct)))
   (t
    '((hilit-inside-bracket-region      ; Removed rm from list
       "\\\\\\(text\\(bf\\|md\\|sf\\|tt\\|sc\\|up\\)\\){" 
       bold)

      (hilit-bracket-region 
       "\\\\\\(\\(page\\|v\\)?ref\\|label\\|index\\|glossary\\|[A-Za-z]*cite[A-Za-z]*\\(\\[.*\\]\\)?\\){" 
       crossref)                            ; things that cross-reference

      (hilit-inside-environment "\\\\begin{\\(equation\\|eqnarray\\)\\(\*\\)?}"
                                glob-struct))))


  ;; FIXME: the following doesn't work.  Tried with nil and default. 
   ;(hilit-inside-bracket-region "\\\\\\(intertext\\|text\\|mbox\\){" default)
  ;; \intertext{<arbitrary text>} will set normal text. 
  ;;  And within any math mode \text{<>} acts like a 'smart' \mbox{}.

  '((hilit-inside-bracket-region "\\\\\\(text\\(it\\|sl\\)\\|emph\\){" italic)

  ;;("\\\\be\\b" "\\\\ee\\b" glob-struct)
    ("\\\\("  "\\\\)" glob-struct)           ; \( \)
    ("[^\\\\\\(\\\\begin{avm}\\)]\\\\\\[" "\\\\\\]" 
     glob-struct) ; \[ \] but not \\[len] or \begin{avm}\[

;;; ("[^$\\]\\($\\($[^$]*\\$\\|[^$]*\\)\\$\\)" 1 formula); '$...$' or '$$...$$'
;;; LB:    ("\\(^\\|[^\\]\\)\\(\\$\\($\\([^\\$]\\|\\\\.\\)*\\$\\|\\([^\\$]\\|\\\\.\\)*\\)\\$\\)"
;;; LB:     2 glob-struct) ; '$...$' or '$$...$$'

    ;; things that bring in external files
    (hilit-bracket-region  "\\\\\\(include\\|input\\|bibliography\\){" include)
    ;; (re-)define new commands/environments/counters

    (hilit-bracket-region 
    "\\\\\\(re\\)?new\\(environment\\|command\\|length\\|theorem\\|counter\\){"
     defun)

    (hilit-bracket-region 
     "\\\\\\(\\(v\\|h\\)space\\|footnote\\(mark\\|text\\)?\\|\\(sub\\)*\\(paragraph\\|section\\)\\|chapter\\|part\\)\\(\*\\)?\\(\\[.*\\]\\)?{"
     keyword)

    (hilit-bracket-region 
     "\\\\\\(title\\|author\\|date\\|thanks\\|address\\)\\(\\[.*\\]\\)?{" 
     define)

    (hilit-bracket-region 
     "\\\\\\(\\(\\this\\)?pagestyle\\|pagenumbering\\|numberwithin\\|begin\\|end\\|nofiles\\|includeonly\\|bibliographystyle\\|document\\(style\\|class\\)\\|usepackage\\)\\(\\[.*\\]\\)?{" 
     decl)
   
    (hilit-bracket-region "\\\\caption\\(\\[[^]]*\\]\\)?{" warning))
  
  (and hilit-LaTeX-commands
       '(("\\(^\\|[^\\\\]\\)\\(\\\\[a-zA-Z\\\\]+\\)" 2 summary-killed)))))


(require 'ispell)

(setq ispell-tex-skip-alists
  '((("%\\[" . "%\\]")
     ;; All the standard LaTeX keywords from L. Lamport's guide:
     ;; \cite, \hspace, \hspace*, \hyphenation, \include, \includeonly, \input,
     ;; \label, \nocite, \rule (in ispell - rest included here)
     ("\\\\addcontentsline"              ispell-tex-arg-end 2)
     ("\\\\add\\(tocontents\\|vspace\\)" ispell-tex-arg-end)
     ("\\\\\\([aA]lph\\|arabic\\)"	 ispell-tex-arg-end)
     ("\\\\author"			 ispell-tex-arg-end)
     ("\\\\bibliographystyle"		 ispell-tex-arg-end)
     ("\\\\makebox"			 ispell-tex-arg-end 0)
;; LB: \figurelisting
     ("\\\\figurelisting[a-z]*"			 ispell-tex-arg-end 3)
;; End of addition
     ;;("\\\\epsfig"		ispell-tex-arg-end)
     ("\\\\document\\(class\\|style\\)" .
      "\\\\begin[ \t\n]*{[ \t\n]*document[ \t\n]*}")
;; Added by LB: \|name|
     ("\\\\|[^|]" . "[^|]|")
     ("\\$Id". "\\$")
     ("^echo". "#")
     ("cat <<". "#")
;; End of addition
     )
    (;; delimited with \begin.  In ispell: displaymath, eqnarray, eqnarray*,
     ;; equation, minipage, picture, tabular, tabular* (ispell)
     ("\\(figure\\|table\\)\\*?"  ispell-tex-arg-end 0)
     ("list"			  ispell-tex-arg-end 2)
     ("shell"		. "\\\\end[ \t\n]*{[ \t\n]*shell[ \t\n]*}")
     ("script"		. "\\\\end[ \t\n]*{[ \t\n]*script[ \t\n]*}")
     ("makefile"		. "\\\\end[ \t\n]*{[ \t\n]*makefile[ \t\n]*}")
     ("program"		. "\\\\end[ \t\n]*{[ \t\n]*program[ \t\n]*}")
     ("verbatim\\*?"	. "\\\\end[ \t\n]*{[ \t\n]*verbatim\\*?[ \t\n]*}"))
    )
)



