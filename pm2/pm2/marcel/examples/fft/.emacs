; .emacs
;                                       Yves Denneulin --- <denneuli@lifl.fr>
;                                       28 Aug 1996
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; %W%      %G%
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;  
; 

;	$Id: .emacs,v 1.1.1.1 1999/11/23 13:40:40 oaumage Exp $	


;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
;;; -*- Mode: Emacs-Lisp -*-
;; ~/.emacs: pour xemacs ou FSF emacs
;;-----------------------------------------------------------------------------
;; Yves Denneulin (pique a JF Roos)
;; 02/01/95
;;-----------------------------------------------------------------------------
;; Ce fichier permet de configurer les differents modules les plus utilises
;; (vm, gnus, auctex,...), et ce pour FSF Emacs et XEmacs.
;;-----------------------------------------------------------------------------
;; 02/01/96 Suppression live-icon
;; 05/01/96 Ajout line-number-mode et visible-bell, hook font-lock pour makefile-mode
;; 05/01/96 Suppression du mode diary
;; 05/01/96 ajout du passage automatique en mode makefile
;; 21/02/96 Ajout du support mime (.vm modifie aussi)
;; 28/02/96 Suppression du decodage MIME automatique avec vm
;; 29/02/96 Suppression du "XEmacs" dans la modeline
;; 03/05/96 Modification de la valeur du threshold pour le GC (canceled 21/06)
;; 28/05/96 Ajout d'une condition xemacs sur tex-toolbar
;;          Ajout du dump de la pile en cas d'erreur
;; 04/06/96 Modification de la valeur de buffers-menu-max-size
;; 04/06/96 Doesn't load sounds anymore (visible-bell set to t)
;; 10/06/96 Added filladapt 2.08 (Kyle)
;; 13/06/96 Added loading of the paren library
;; 20/08/96 Disable fast-lock (problem with GNU emacs)
;; 21/08/96 Add (require 'crypt++) in GNU emacs configuration
;; 22/08/96 Add html-helper-mode in GNU configuration and activate font-lock
;; 22/08/96 Suppression of "Emacs" in the mode-line
;; 22/08/96 Modification of the gont-lock-maximum-decoration variable
;; 26/08/96 Add support for idl-mode (still faulty)
;; 27/08/96 Add global font-lock 
;;          Add auto-insert package (phM+Boris) (for GNU emacs only)
;;          Add version-control (Boris)
;; 04/09/96 Prints on hpps by default
;; 10/12/96 Comment out the iso-tex declaration
;; 07/02/97 Don't use TM anymore
;; 02/04/97 Erase diary entries
;; 25/05/97 Ispell : french now default dictionary
;;-----------------------------------------------------------------------------


;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------
;; configuration generale
;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------
(ispell-change-dictionary "francais")
(load-library "paren")
(setq stack-trace-on-error t)
(setq gc-cons-threshold 500000)
(setq visible-bell t)
(line-number-mode 1)
(setq-default overwrite-mode nil)
(setq-default debug-on-error nil)
(setq-default debug-on-quit nil)
(setq-default buffers-menu-max-size nil)
(setq-default case-fold-search t)
(setq-default case-replace t)
;; Font lock sur tous les buffers
(global-font-lock-mode t)

(setq-default truncate-lines nil)
(setq-default mouse-yank-at-point nil)
(setq-default font-lock-maximum-decoration t)
(setq auto-mode-alist 
      (append '(("[Mm]akefile.*" . makefile-mode))
	      '((".*[Mm]ak" . makefile-mode))
	      '(("\\.ad.?$" . ada-mode))
	      '(("\\.idl$" . idl-mode))
	      auto-mode-alist))
(setq ps-lpr-switches '("-dhpps"))
;------------------------------------------------------------------------------
; Version control RCS 
(setq vc-make-backup-files t) ; produire les *~
(setq vc-command-messages t) ; verbose 
(setq vc-default-back-end 'RCS) ; 

;;--------------------------------------------------------------------------
;; Touches de fonction pour differents claviers
;;-----------------------------------------------------------------------------
;; clavier Sun
(define-key global-map [home] 'beginning-of-line)
(define-key global-map [end] 'end-of-line)
(define-key global-map [f18] 'yank)              ;; paste
(define-key global-map [f17] 'iconify-or-deiconify-frame)     ;; open
(define-key global-map [f16] 'kill-ring-save)    ;; copy
(define-key global-map [f20] 'kill-region)       ;; cut
(define-key global-map [f12] 'yank-pop)          ;; again
(define-key global-map [f19] 'isearch-forward)   ;; find
(define-key global-map [f14] 'undo)              ;; undo
(define-key global-map [f29] 'scroll-down)       ;; Pg Up
(define-key global-map [f35] 'scroll-up)         ;; Pg Dn
(define-key global-map [f33] 'end-of-line)       ;; End
(define-key global-map [f27] 'beginning-of-line) ;; Home
(define-key global-map [f31] 'recenter)          ;; 5
(define-key global-map [f1] 'gnus)
(define-key global-map [f2] 'vm)
(define-key global-map [f3] 'shell)
(define-key global-map [f4] 'calendar)


;;-----------------------------------------------------------------------------
;; racourcis, commandes frequentes
;;-----------------------------------------------------------------------------
(global-set-key "\C-cl" 'goto-line)     ; aller a une ligne donnee
(global-set-key "\C-c\C-m" 'compile)    ; compiler
(put 'narrow-to-region 'disabled nil)
(put 'eval-expression 'disabled nil)


;;-----------------------------------------------------------------------------
;; Xemacs ou Emacs
;;-----------------------------------------------------------------------------
(defvar running-xemacs (string-match "XEmacs\\|Lucid" emacs-version))


;(cond ((not running-xemacs) (setq mode-line-inverse-video nil)))



;;---------------------------------------------------------------------------
;;---------------------------------------------------------------------------
;; code pour toute version de XEmacs
;;---------------------------------------------------------------------------
;;---------------------------------------------------------------------------
(cond (running-xemacs
       ;;--------------------------------------------------------------------
       ;; variables diverses
       ;;--------------------------------------------------------------------
       (setq find-file-use-truenames nil
	     find-file-compare-truenames t
	     minibuffer-confirm-incomplete t
	     complex-buffers-menu-p t
	     next-line-add-newlines nil
	     kill-whole-line t
	     mail-yank-prefix "> "
	     )

       (setq-default teach-extended-commands-p nil)
       (setq-default bar-cursor 2)
       (setq-default complex-buffers-menu-p t)
       (setq-default font-menu-ignore-scaled-fonts t)
       (setq-default font-menu-this-frame-only-p t)
       (setq-default zmacs-regions t)

       ;; Make F6 be `save-file' followed by `delete-window'.
       (global-set-key 'f6 "\C-x\C-s\C-x0")


       ;;----------------------------------------------------------------------
       ;; mode line buffer
       ;;----------------------------------------------------------------------
       (if (boundp 'modeline-buffer-identification)
	   (progn
	     (setq-default modeline-buffer-identification '(" %17b"))
	     (setq modeline-buffer-identification '(" %17b")))
;;	     (setq-default modeline-buffer-identification '("XEmacs: %17b"))
;;	     (setq modeline-buffer-identification '("XEmacs: %17b")))
	 (setq-default mode-line-buffer-identification '(" %17b"))
	 (setq mode-line-buffer-identification '(" %17b")))
;;	 (setq-default mode-line-buffer-identification '("XEmacs: %17b"))
;;	 (setq mode-line-buffer-identification '("XEmacs: %17b")))


       ;;----------------------------------------------------------------------
       ;; sous X
       ;;----------------------------------------------------------------------
       (cond ((or (not (fboundp 'device-type))
		  (equal (device-type) 'x))
	      ;;---------------------------------------------------------------
	      ;; pour quitter XEmacs
	      ;;---------------------------------------------------------------
	      (defun query-kill-emacs ()
		"Asks if you want to quit emacs before quiting."
		(interactive)
		(if (yes-or-no-p-dialog-box "Are you sure you want to quit? ")
		    (save-buffers-kill-emacs)
		  (message "Didn't think so ... :-)"))
		)
	      (global-set-key "\C-x\C-c"  'query-kill-emacs)

	      ;;---------------------------------------------------------------
	      ;; sticky modifier keys enabled
	      ;;---------------------------------------------------------------
	      ;;(setq modifier-keys-are-sticky t)

	      ;;---------------------------------------------------------------
	      ;; title bar
	      ;;---------------------------------------------------------------
	      (if (equal frame-title-format "%S: %b")
		  (setq frame-title-format
			(concat "%S: " invocation-directory invocation-name
				" [" emacs-version "]"
				(if nil ; (getenv "NCD")
				    ""
				  "   %b"))))

	      ;;---------------------------------------------------------------
	      ;; sounds
	      ;;---------------------------------------------------------------
;	      (cond ((string-match ":0" (getenv "DISPLAY"))
;		     (load-default-sounds))
;		    (t
;		     (setq bell-volume 40)
;		     (setq sound-alist
;			   (append sound-alist '((no-completion :pitch 500))))
;		     ))

	      ;;---------------------------------------------------------------
	      ;; Change the cursor used when the mouse is over a mode line
	      ;;---------------------------------------------------------------
	      (setq x-mode-pointer-shape "leftbutton")

	      ;;---------------------------------------------------------------
	      ;; Change the cursor used during garbage collection.
	      ;;---------------------------------------------------------------
	      (if (featurep 'xpm)
		  (let ((file (expand-file-name "recycle.xpm" data-directory)))
 		    (if (condition-case error
			    (make-cursor file)		; returns a cursor if successful.
 			  (error nil))			; returns nil if an error occurred.
			(setq x-gc-pointer-shape file))) )
	      
	      ;;---------------------------------------------------------------
	      ;; Change the binding of mouse button 2, so that it inserts the
	      ;; selection at point (where the text cursor is), instead of at
	      ;; the position clicked.
	      ;;---------------------------------------------------------------
	      (global-set-key 'button2 
			      '(lambda () (interactive) (x-insert-selection t)))


	      ;;---------------------------------------------------------------
	      ;; Menus
	      ;;---------------------------------------------------------------
	      ;; Add `dired' to the File menu
	      (add-menu-item '("File") "Edit Directory" 'dired t)
	      
	      ;; Here's a way to add scrollbar-like buttons to the menubar
	      (add-menu-item nil "Top" 'beginning-of-buffer t)
	      (add-menu-item nil "<<<" 'scroll-down t)
	      (add-menu-item nil " . " 'recenter t)
	      (add-menu-item nil ">>>" 'scroll-up t)
	      (add-menu-item nil "Bot" 'end-of-buffer t)

	      ;;---------------------------------------------------------------
	      ;; LISPM bindings of Control-Shift-C and Control-Shift-E.
	      ;;---------------------------------------------------------------
	      (define-key emacs-lisp-mode-map '(control C) 'compile-defun)
	      (define-key emacs-lisp-mode-map '(control E) 'eval-defun)


	      ;;---------------------------------------------------------------
	      ;; Make backspace and delete be the same.  This doesn't work in all
	      ;; cases; a better way would be to use xmodmap.
	      ;;---------------------------------------------------------------
	      (global-set-key 'backspace [delete])
	      (global-set-key '(meta backspace) [(meta delete)])
	      (global-set-key '(control backspace) [(control delete)])
	      (global-set-key '(meta control backspace) [(meta control delete)])

	      ;;---------------------------------------------------------------
	      ;; accents sous X
	      ;;---------------------------------------------------------------
	      (require 'x-compose)
	      ))

       ;;---------------------------------------------------------------
       ;; resize-minibuffer-mode makes the minibuffer automatically
       ;; resize as necessary when it's too big to hold its contents
       ;;---------------------------------------------------------------
       (autoload 'resize-minibuffer-mode "rsz-minibuf" nil t)
       (resize-minibuffer-mode)
       (setq resize-minibuffer-window-exactly nil)

       (add-spec-list-to-specifier modeline-shadow-thickness '((global (nil . 2))))
       (require 'pending-del)
;       (load "mime-setup")
;       (setq tm-vm/automatic-mime-preview nil)
       ))



;;-----------------------------------------------------------------------------
;; Quelle version d'emacs?
;;-----------------------------------------------------------------------------

(if (and (not (boundp 'emacs-major-version))
	 (string-match "^[0-9]+" emacs-version))
    (setq emacs-major-version
	  (string-to-int (substring emacs-version
				    (match-beginning 0) (match-end 0)))))
(if (and (not (boundp 'emacs-minor-version))
	 (string-match "^[0-9]+\\.\\([0-9]+\\)" emacs-version))
    (setq emacs-minor-version
	  (string-to-int (substring emacs-version
				    (match-beginning 1) (match-end 1)))))

;;; Define a function to make it easier to check which version we're
;;; running.

(defun running-emacs-version-or-newer (major minor)
  (or (> emacs-major-version major)
      (and (= emacs-major-version major)
	   (>= emacs-minor-version minor))))





;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------
;; code pour XEmacs 19 6 ou plus recent
;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------

(cond ((and running-xemacs
	    (running-emacs-version-or-newer 19 6))
       ;;
       ;; Code requiring XEmacs/Lucid Emacs version 19.6 or newer goes here
       ;;
       ))

;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------
;; code pour GNU emacs 19
;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------

(cond ((and (not running-xemacs)
	    (>= emacs-major-version 19))
       ;;----------------------------------------------------------------------
       ;; accents
       ;;----------------------------------------------------------------------
       (setq iso-accents-mode t)
       (standard-display-european t) ;;; affichage des caracteres 8 bits
       (require 'iso-syntax )        ;;; les nouveaux caracteres sont des lettres
       (require 'iso-transl )        ;;; C-x 8 comme Compose


       ;;----------------------------------------------------------------------
       ;; recherche du code lisp
       ;;----------------------------------------------------------------------
       (setq load-path  (cons "/usr/local/liptools/lib/emacs/site-lisp/auctex"  load-path))
       (setq load-path  (cons "/usr/local/liptools/lib/emacs/site-lisp"  load-path))
       (setq load-path (cons "/home/rnamyst/usr/emacs/site-lisp" load-path))
       ;;----------------------------------------------------------------------
       ;; couleurs
       ;;----------------------------------------------------------------------
       (transient-mark-mode 1)	;;; affichage en grise de la selection

       ;;----------------------------------------------------------------------
       ;; html-helper-mode
       ;;----------------------------------------------------------------------
       (setq auto-mode-alist
	     (append '(("\\.html$" . html-helper-mode)
		       ) auto-mode-alist))
       (autoload 'html-helper-mode "html-helper-mode" "Yay HTML" t)

       ;; Changement dans la modeline
       (setq-default mode-line-buffer-identification '("%12b"))

       ;; Positionne le niveau de decoration pour chaque mode
       (setq font-lock-maximum-decoration
	     '( (c++-mode . t) (c-mode . t) (lisp-mode . t) 
		(html-helper-mode . t) (t . t)
		))
       ;;---------------------------------------------------------------------
       ;; Webster
       ;;
       (autoload 'webster "webster19" "look up a word in Webster's 7th edition" t)
       (setq webster-host '("relay-csri-ether.cs.toronto.edu"))

       ; -----------------------------------------------------------------
       ; Installation du menu fonction
       ; -----------------------------------------------------------------
       (require 'func-menu)
       (define-key global-map [f8] 'function-menu)
       (add-hook 'find-file-hooks 'fume-add-menubar-entry)
       (define-key global-map "\C-cl" 'fume-list-functions)
       (define-key global-map "\C-cg" 'fume-prompt-function-goto)
       (define-key global-map [S-down-mouse-3] 'mouse-function-menu)
       (define-key global-map [M-down-mouse-1] 'fume-mouse-function-goto)

       ;; ------------------------------------------------------------
       ;; Utilisation de tm
       ;; ------------------------------------------------------------
;       (setq load-path
;	     (cons "/usr3/GNUemacs/share/emacs/site-lisp/tm-7.89"  load-path))
;      (load "mime-setup")
       ))


;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------
;;		Customization of Specific Packages		    
;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------



;;-----------------------------------------------------------------------------
;; This adds additional extensions which indicate files normally
;; handled by cc-mode.
;;-----------------------------------------------------------------------------
(autoload 'c++-mode  "cc-mode" "C++ Editing Mode" t)
(autoload 'c-mode    "cc-mode" "C Editing Mode" t)
(autoload 'objc-mode "cc-mode" "Objective-C Editing Mode" t)
(autoload 'idl-mode "idl-mode" "IDL editing mode" t)
(setq auto-mode-alist
      (append '(("\\.C$"  . c++-mode)
		("\\.cc$" . c++-mode)
		("\\.hh$" . c++-mode)
		("\\.c$"  . c-mode)
		("\\.h$"  . c-mode)
		("\\.m$"  . objc-mode)
		) auto-mode-alist))

;; This controls indentation.  The default is 4 spaces but the
;; Emacs source code uses 2.
(setq c-basic-offset 2)





;;-----------------------------------------------------------------------------
;; VM
;;-----------------------------------------------------------------------------
(autoload 'vm "vm" "Start VM on your primary inbox." t)
(autoload 'vm-visit-folder "vm" "Start VM on an arbitrary folder." t)
(autoload 'vm-visit-virtual-folder "vm" "Visit a VM virtual folder." t)
(autoload 'vm-mode "vm" "Run VM major mode on a buffer" t)
(autoload 'vm-mail "vm" "Send a mail message using VM." t)
(autoload 'vm-submit-bug-report "vm" "Send a bug report about VM." t)
(cond (running-xemacs
       (require 'highlight-headers)
       (set-face-foreground 'message-headers "darkslateblue")
       (set-face-foreground 'message-header-contents "brown")
       (set-face-foreground 'message-highlighted-header-contents "black")
       (set-face-font 'message-highlighted-header-contents
		      "-adobe-courier-bold-r-normal--12*")
       (set-face-foreground 'message-cited-text "darkgreen")
       ))



;;-----------------------------------------------------------------------------
;; accents sous TeX
;;-----------------------------------------------------------------------------
;(autoload 'iso-tex-minor-mode  "iso-tex"
;  "Translate TeX to ISO 8859/1 while visiting a file." t)
;(autoload 'iso2tex  "iso-tex"  "Translate ISO 8859/1 to TeX." t)
;(autoload 'tex2iso  "iso-tex"  "Translate TeX to ISO 8859/1." t)




;;-----------------------------------------------------------------------------
;; bibtex
;;-----------------------------------------------------------------------------
(setq auto-bibtex-x-environment nil)
(cond (running-xemacs
       (require 'bib-cite)
       ))



;;-----------------------------------------------------------------------------
;; auc-tex v9
;;-----------------------------------------------------------------------------
(defadvice set-text-properties (around ignore-strings activate)
               "Ignore strings."
               (or (stringp (ad-get-arg 3))
                   ad-do-it))

(setq LaTeX-mode-hook  
'(lambda ()
   ;; (LaTeX-math-mode) 
   ;; (abbrev-mode nil) 
   (auto-fill-mode nil)
   (iso-tex-minor-mode 1)
   ) )

(cond (running-xemacs
       (require 'tex-toolbar)))
(require 'tex-site)


;; labels, environnements et indentations
(setq LaTeX-figure-label "fig-")
(setq LaTeX-table-label "tab-")
(setq LaTeX-section-label "sec-")
(setq LaTeX-default-options "a4paper")
(setq LaTeX-float "htbp")
(setq LaTeX-default-format "rcl")
(setq LaTeX-default-environment "itemize")
(setq LaTeX-default-style "article")
(setq TeX-electric-escape nil)
(setq TeX-default-macro "mbox")
(setq LaTeX-indent-level 4)
(setq LaTeX-brace-indent-level 5)
(setq LaTeX-item-indent -2)

;; multifile documents
(setq-default TeX-master nil)
(setq TeX-save-document t)

;; parsing
(setq TeX-parse-self t)
(setq TeX-auto-save t)

;;  outline-minor-mode
(setq outline-minor-mode-prefix "\C-c\C-o") 
(autoload 'outline-minor-mode "min-out" "Minor Outline Mode." t)
(global-set-key "\C-co" 'outline-minor-mode)
(make-variable-buffer-local 'outline-prefix-char)
(setq-default outline-prefix-char "\C-l")
(make-variable-buffer-local 'outline-regexp)
(setq-default outline-regexp "[*\^l]+");; this is purely a default
(make-variable-buffer-local 'outline-level-function)
(setq-default outline-level-function 'outline-level-default)

;; impression
(defvar TeX-printer-list 
'(
  ("ps" "dvips -f %s | lp -d%p" "lpstat -o all") ; forme generale 
  ("lwi2")
  ("hpps")
  ("hp4siv")
  ("hp4si")
  ("lexmark")
  ("lexmarkv")
  ))

(defvar TeX-default-printer-name "lwi2")

;; Preview 
;; en fonction de ce que l'on trouve dans les styles LaTeX du fichier 
(defvar TeX-view-style 
'(
  ("^epsf.*booklet$" "xdvi %d -paper a4r")
  ("^booklet.*epsf$" "xdvi %d -paper a4r")
  ("^a5$" "xdvi %d -paper a5")
  ("^booklet$" "xdvi %d -paper a4r")
  ("^landscape$" "xdvi %d -paper a4r")
  ("^myslide.hp$" "xdvi %d -paper a4r")
  ("^lslide$" "xdvi %d -paper a4r")
  ("." "xdvi %d"))
"*List of style options and view options.
If the first element (a regular expresion) matches the name of one of
the style files, any occurrence of the string %v in a command in
TeX-command-list will be replaced with the second element.  The first
match is used, if no match is found the %v is replaced with the empty
string.")



;;-----------------------------------------------------------------------------
;; gnus
;;-----------------------------------------------------------------------------
(setq gnus-nntp-server "news.univ-lille1.fr")
(setq gnus-local-domain "lifl.fr")
(setq gnus-local-organization "Laboratoire d'Informatique Fondamentale de Lille")
(setq gnus-nntp-service 119)
(setq gnus-local-timezone "MET")
(setq gnus-novice-user t)

;; chargement a la demande 
(autoload 'gnus-xemacs "gnus-xemacs" "Read network news." t)
(autoload 'gnus "gnus" "Read network news." t)
(autoload 'gnus-post-news "gnuspost" "Post a news." t)
(autoload 'gnus "gnus-speedups" "Accelaration de gnus" t)
(autoload 'gnus "gnus-mark" "operate on more than one article at a time" t)

;; lecture
(setq gnus-use-cross-reference t)
(setq gnus-large-newsgroup 100)
(setq gnus-interactive-catchup t)
(setq gnus-show-threads t)
(setq gnus-thread-hide-subject t)
(setq gnus-thread-hide-subtree nil)
(setq gnus-subscribe-newsgroup-method (function gnus-subscribe-interactively ))

;; post d'articles
(setq gnus-author-copy "~/maildir/News/record-news")
(setq gnus-signature-file "~/templates/signature")
(setq gnus-interactive-post t)

;; sauvegarde d'articles
(setq gnus-use-long-file-name t)
(setq gnus-article-save-directory "~/maildir/News")
(setq gnus-default-article-saver (function gnus-summary-save-in-file))
(setq gnus-file-save-name (function gnus-plain-save-name))

;; big brother
;(setq gnus-startup-hook 'bbdb-insinuate-gnus)




;;-----------------------------------------------------------------------------
;; ISPELL
;;-----------------------------------------------------------------------------
;; When running ispell, consider all 1-3 character words as correct.
(setq ispell-extra-args '("-W" "3"))
(autoload 'ispell-word "ispell"  "Check the spelling of word in buffer." t)
(global-set-key "\e$" 'ispell-word)
(autoload 'ispell-region "ispell"  "Check the spelling of region." t)
(autoload 'ispell-buffer "ispell"  "Check the spelling of buffer." t)
;(global-set-key "\e#" 'ispell-buffer)
(autoload 'ispell-complete-word "ispell"
  "Look up current word in dictionary and try to complete it." t)
(global-set-key "\e*" 'ispell-complete-word)
(autoload 'ispell-change-dictionary "ispell"  "Change ispell dictionary." t)
(autoload 'ispell-message "ispell"  "Check spelling of mail message or news post.")




;;-----------------------------------------------------------------------------
;; Big Brother Data Base : bbdb
;;-----------------------------------------------------------------------------
(global-set-key "\C-cb" 'bbdb)

(autoload 'bbdb         "bbdb-com" "Insidious Big Brother Database" t)
(autoload 'bbdb-name    "bbdb-com" "Insidious Big Brother Database" t)
(autoload 'bbdb-company "bbdb-com" "Insidious Big Brother Database" t)
(autoload 'bbdb-net     "bbdb-com" "Insidious Big Brother Database" t)
(autoload 'bbdb-notes   "bbdb-com" "Insidious Big Brother Database" t)
(autoload 'bbdb-insinuate-vm       "bbdb-vm"     "Hook BBDB into VM")
(autoload 'bbdb-insinuate-rmail     "bbdb-rmail" "Hook BBDB into RMAIL")
(autoload 'bbdb-insinuate-gnus      "bbdb-gnus"  "Hook BBDB into GNUS")
(autoload 'bbdb-insinuate-sendmail "bbdb"        "Hook BBDB into sendmail")

(defun bbdb-temp ()
  (bbdb-insinuate-sendmail)
  (bbdb-define-all-aliases)
)        

;(setq mail-setup-hook
;      '(lambda ()
;	 (bbdb-insinuate-sendmail)
;	 (bbdb-define-all-aliases))
;)                                 
(add-hook 'mail-setup-hook 'bbdb-temp)

(setq bbdb-file "~/info/bbdb")
;(setq bbdb-default-area-code 33)
;(setq bbdb-north-american-phone-numbers-p nil) 
(setq bbdb-user-mail-names ".*[Dd]enneulin.*")
(setq bbdb-canonicalize-redundant-nets-p t) 
(setq bbdb-message-caching-enabled t) 
;(setq bbdb-info-file "/usr1/gnu/lib/emacs/site-lisp/bbdb/bbdb.info")
;;; pas de creation automatique 
;(setq bbdb/mail-auto-create-p nil)
(setq bbdb/news-auto-create-p nil)
;;; tj affiche
(setq bbdb-use-pop-up t)
(setq bbdb-completion-display-record nil)



(cond (running-xemacs



       ;;----------------------------------------------------------------------
       ;; Auto Save
       ;;----------------------------------------------------------------------
       (setq auto-save-directory (expand-file-name "~/.autosaves/")
	     auto-save-directory-fallback auto-save-directory
	     auto-save-hash-p nil
	     ange-ftp-auto-save t
	     ange-ftp-auto-save-remotely nil
	     ;; now that we have auto-save-timeout, let's crank this up
	     ;; for better interactive response.
	     auto-save-interval 2000
	     )
       (require 'auto-save)
       
       ;;----------------------------------------------------------------------
       ;; crypt
       ;;----------------------------------------------------------------------
       (setq crypt-encryption-type 'pgp	; default encryption mechanism
	     crypt-confirm-password t		; make sure new passwords are correct
	     )
       (require 'crypt)

))


;;-----------------------------------------------------------------------------
;; Ange FTP
;;-----------------------------------------------------------------------------
(require 'dired)
(require 'ange-ftp)
(setq ange-ftp-default-user "anonymous"      ; id to use for /host:/remote/path
      ange-ftp-generate-anonymous-password (concat (getenv "USER") "@lifl.fr") 
      ange-ftp-binary-file-name-regexp "."   ; always transfer in binary mode
      ange-ftp-tmp-name-template "/var/tmp/ange-ftp"
      )

;;-----------------------------------------------------------------------------
;; font-lock
;;-----------------------------------------------------------------------------
(cond (running-xemacs
       (setq-default font-lock-auto-fontify t)
       (setq-default font-lock-use-fonts nil)
       (setq-default font-lock-use-colors t)
       (setq-default font-lock-use-maximal-decoration t)
       (setq-default font-lock-mode-enable-list nil)
       (setq-default font-lock-mode-disable-list nil)
       ))
(require 'font-lock)
(cond (running-xemacs
       ;(set-face-foreground 'font-lock-comment-face "red")
;       (set-face-foreground 'font-lock-comment-face "#6920ac")
;       (set-face-background 'font-lock-comment-face "LightGrey")
;       (set-face-font 'font-lock-comment-face "-adobe-*-medium-r-*-*-16-*-*-*-m-*-iso8859-1")
;       ;(set-face-foreground 'font-lock-string-face "forest green")
;       (set-face-foreground 'font-lock-string-face "green4")
;       (set-face-background 'font-lock-string-face "LightGrey")
;       (set-face-font 'font-lock-string-face "-adobe-*-medium-r-*-*-16-*-*-*-m-*-iso8859-1")
;       (set-face-foreground 'font-lock-doc-string-face "green4")
;       (set-face-background 'font-lock-doc-string-face "LightGrey")
;       (set-face-font 'font-lock-doc-string-face "-adobe-*-medium-r-*-*-16-*-*-*-m-*-iso8859-1")
;       ;(set-face-foreground 'font-lock-function-name-face "blue")
;       (set-face-foreground 'font-lock-function-name-face "red3")
;       (set-face-background 'font-lock-function-name-face "LightGrey")
;       (set-face-font 'font-lock-function-name-face "-adobe-*-medium-r-*-*-16-*-*-*-m-*-iso8859-1")
;       (set-face-foreground 'font-lock-keyword-face "blue3")
;       (set-face-background 'font-lock-keyword-face "LightGrey")
;       (set-face-font 'font-lock-keyword-face "-adobe-*-medium-r-*-*-16-*-*-*-m-*-iso8859-1")
;       (set-face-foreground 'font-lock-type-face "blue3")
;       (set-face-background 'font-lock-type-face "LightGrey")
;       (set-face-font 'font-lock-type-face "-adobe-*-medium-r-*-*-16-*-*-*-m-*-iso8859-1")
       ))
(add-hook 'emacs-lisp-mode-hook	'turn-on-font-lock)
(add-hook 'lisp-mode-hook	'turn-on-font-lock)
(add-hook 'c-mode-hook		'turn-on-font-lock)
(add-hook 'c++-mode-hook	'turn-on-font-lock)
(add-hook 'perl-mode-hook	'turn-on-font-lock)
(add-hook 'tex-mode-hook	'turn-on-font-lock)
(add-hook 'LaTeX-mode-hook      'turn-on-font-lock)
(add-hook 'latex-mode-hook      'turn-on-font-lock)
(add-hook 'bibtex-mode-hook	'turn-on-font-lock)
(add-hook 'texinfo-mode-hook	'turn-on-font-lock)
(add-hook 'postscript-mode-hook	'turn-on-font-lock)
(add-hook 'dired-mode-hook	'turn-on-font-lock)
(add-hook 'ada-mode-hook	'turn-on-font-lock)
(add-hook 'makefile-mode-hook   'turn-on-font-lock)
(add-hook 'html-helper-mode-hook 'turn-on-font-lock)

;;-----------------------------------------------------------------------------
;; fast-lock
;;-----------------------------------------------------------------------------
;(add-hook 'font-lock-mode-hook 'turn-on-fast-lock)
;(setq fast-lock-cache-directories '("~/.fast-lock"))



(cond (running-xemacs
       
       ;;-----------------------------------------------------------------------------
       ;; func-menu
       ;;-----------------------------------------------------------------------------
       (require 'func-menu)
       (define-key global-map 'f8 'function-menu)
       (add-hook 'find-file-hooks 'fume-add-menubar-entry)
       (define-key global-map "\C-cg" 'fume-prompt-function-goto)
       (define-key global-map '(shift button3) 'mouse-function-menu)


       ;;-----------------------------------------------------------------------------
       ;; OO-Browser
       ;;-----------------------------------------------------------------------------
       (autoload 'oobr "oobr/br-start" "Invoke the OO-Browser" t)
       (autoload 'br-env-browse "oobr/br-start"
	 "Browse an existing OO-Browser Environment" t)
       (global-set-key "\C-c\C-o" 'oobr)


       ;;-----------------------------------------------------------------------------
       ;; completion multiple
       ;;-----------------------------------------------------------------------------
       (load-library "completer")


       ;;-----------------------------------------------------------------------------
       ;; live-icon
       ;;-----------------------------------------------------------------------------
       ;(require 'live-icon)

       ))



;;-----------------------------------------------------------------------------
;; Edebug is a source-level debugger for emacs-lisp programs.
;;-----------------------------------------------------------------------------
(define-key emacs-lisp-mode-map "\C-xx" 'edebug-defun)


;;-----------------------------------------------------------------------------
;; html-mode (for Xemacs only)
;;-----------------------------------------------------------------------------
(cond (running-xemacs
       (autoload 'html-mode "html-mode" "HTML major mode." t)
       (or (assoc "\\.html$" auto-mode-alist)
	   (setq auto-mode-alist (cons '("\\.html$" . html-mode) 
				       auto-mode-alist)))
))


;;-----------------------------------------------------------------------------
;; w3
;;-----------------------------------------------------------------------------
(setq w3-default-homepage "http://www.lifl.fr/")
(setq load-path (cons (expand-file-name "THELISPDIR") load-path))
(autoload 'w3-preview-this-buffer "w3" "WWW Previewer" t)
(autoload 'w3-follow-url-at-point "w3" "Find document at pt" t)
(autoload 'w3 "w3" "WWW Browser" t)
(autoload 'w3-open-local "w3" "Open local file for WWW browsing" t)
(autoload 'w3-fetch "w3" "Open remote file for WWW browsing" t)
(autoload 'w3-use-hotlist "w3" "Use shortcuts to view WWW docs" t)
(autoload 'w3-show-hotlist "w3" "Use shortcuts to view WWW docs" t)
(autoload 'w3-follow-link "w3" "Follow a hypertext link." t)
(autoload 'w3-batch-fetch "w3" "Batch retrieval of URLs" t)
(autoload 'url-get-url-at-point "url" "Find the url under the cursor" nil)
(autoload 'url-file-attributes  "url" "File attributes of a URL" nil)
(autoload 'url-popup-info "url" "Get info on a URL" t)
(autoload 'url-retrieve   "url" "Retrieve a URL" nil)
(autoload 'url-buffer-visiting "url" "Find buffer visiting a URL." nil)
(autoload 'gopher-dispatch-object "gopher" "Fetch gopher dir" t)


;;-----------------------------------------------------------------------------
;; filladapt
;;-----------------------------------------------------------------------------
(require 'filladapt)
(setq-default filladapt-mode t)
(add-hook 'text-mode-hook 'turn-on-filladapt-mode)
(add-hook 'c-mode-hook 'turn-off-filladapt-mode)
(add-hook 'mail-mode-hook 'turn-on-filladapt-mode)
;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------
;; demarrage
;;-----------------------------------------------------------------------------
;;-----------------------------------------------------------------------------

(display-time)
(cond (running-xemacs
;       (appt-initialize)
       (gnuserv-start)
       )
      (t (server-start)))

(setq minibuffer-max-depth 5)

;; Options Menu Settings
;; =====================
(cond
 ((and (string-match "XEmacs" emacs-version)
       (boundp 'emacs-major-version)
       (or (and
            (= emacs-major-version 19)
            (>= emacs-minor-version 14))
           (= emacs-major-version 20))
       (fboundp 'load-options-file))
  (load-options-file "~/.xemacs-options")))
;; ============================
;; End of Options Menu Settings

(put 'downcase-region 'disabled nil)
