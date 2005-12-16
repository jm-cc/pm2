(TeX-add-style-hook "start"
 (function
  (lambda ()
    (LaTeX-add-labels
     "sec:tools"
     "subsec:compiling"
     "subsec:configuring"
     "sec:rank"
     "sec:output"
     "sec:self-migration"
     "sec:startupfunc"
     "sec:tradimain"
     "sec:scripts"
     "sec:commonpitfalls"
     "tbl:pitfalls"
     "sec:faq")
    (TeX-run-style-hooks
     "start"
     "latex2e"
     "rep11"
     "report"
     "a4paper"
     "11pt"))))

