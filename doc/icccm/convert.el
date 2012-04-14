(defun convert ()
  (interactive)
  (delete-region (point-min) (re-search-forward "^\.nH"))
  (insert ".nH")
  (mapcar
   (function (lambda (fn)
	       (goto-char (point-min))
	       (message (symbol-name fn))
	       (funcall fn)))
   '(convert-nh convert-nt convert-special convert-rs convert-pn convert-bp convert-ip convert-fs convert-qp convert-cleanup-in-re
		convert-lp convert-special-2 convert-ds convert-sm convert-tables convert-move-footnotes convert-math convert-final-cleanup
		convert-hyperlinks)))

(defun bsub (level)
  (buffer-substring (match-beginning level) (match-end level)))

(defun convert-nh ()
  (let ((counter (make-vector 6 0)))
    (while (re-search-forward "^\\.nH \\([0-9]\\)" () t)
      (let* ((level-string (bsub 1))
	     (level (string-to-number level-string)))
	(aset counter level (1+ (aref counter level)))
	(aset counter (1+ level) 0)
	(replace-match "\n<H\\1>")
	(let ((section-number (let ((index (aref counter 1)))
				(if (< index 9)
				    (number-to-string index)
				  (char-to-string (+ ?A (- index 9))))))
	      (i 1))
	  (while (< i level)
	    (setq i (1+ i))
	    (setq section-number (concat section-number "." (aref counter i))))
	  (insert "<A NAME=\"s-" section-number "\">" section-number ".</A> "))
	(if (looking-at " +\"\\(.*\\)\"")
	    (replace-match "\\1"))
	(end-of-line)
	(insert "</H" level-string ">")
	(forward-char 1)
	(while (looking-at "\\.IN")
	  (kill-line 1))
	(if (looking-at "\\.LP\n")
	    (replace-match "")
	  (error "convert-nh"))))))

(defun convert-lp ()
  (while (re-search-forward "^\\.LP[ \t]*$" () t)
    (replace-match "<P>")))

(defun convert-one-nt-without-ip ()
  (goto-char start)
  (kill-word 1)
  (if (looking-at " \"\\(.*\\)\"$")
      (progn (replace-match "\\1")
	     (beginning-of-line)))
  (insert "<P ALIGN=center>")
  (forward-line 1)
  (beginning-of-line)
  (insert "<BLOCKQUOTE>\n<P>")
  (goto-char end)
  (beginning-of-line)
  (kill-line 1)
  (insert "</BLOCKQUOTE>\n"))

(defun convert-one-nt-with-ip ()
  (goto-char start)
  (kill-word 1)
  (insert "<OL>\n<P ALIGN=center>")
  (end-of-line)
  (insert "<P>")
  (while (re-search-forward "^\\.IP[ \t]*[0-9]" end t)
    (beginning-of-line)
    (kill-line)
    (insert "<LI> ")
    (kill-line))
  (goto-char end)
  (beginning-of-line)
  (kill-line 1)
  (insert "</OL>\n"))
    

(defun convert-one-nt ()
  (let ((start (point))
	(end (copy-marker (re-search-forward "^\\.NE$"))))
    (goto-char start)
    (if (re-search-forward "^\.IP" end t)
	(convert-one-nt-with-ip)
      (convert-one-nt-without-ip))))

(defun convert-nt ()
  (while (re-search-forward "^\\.NT" () t)
    (beginning-of-line)
    (convert-one-nt)))

(defun convert-one-ip-block-unordered ()
  (catch 'exit
    (replace-match "<UL>\n<LI> ")
    (while (re-search-forward "^\\(\\.IP\\|\\.LP\\|<H\\|<P\\)" () t)
      (beginning-of-line)
      (cond ((looking-at "\\.IP \\\\(bu 5")
	     (kill-line 1)
	     (insert "<LI> "))
	    ((looking-at "\\.IP$")
	     (replace-match "<P>"))
	    ((looking-at "\\.LP")
	     (kill-line 1)
	     (throw 'exit ()))
	    ((looking-at "<")
	     (throw 'exit ()))
	    ((error "convert-one-ip-block-unordered"))))
    (re-search-forward "^\\.bp\n")
    (replace-match ""))
  (insert "</UL>\n"))

(defun convert-one-ip-block-numbered ()
  (catch 'exit
    (while (re-search-forward "^\\(\\.IP\\|\\.LP\\|<H\\|<P\\)")
      (beginning-of-line)
      (cond ((looking-at "\\.IP [0-9]\\. 5")
	     (kill-line 1)
	     (insert "<LI> "))
	    ((looking-at "\\.IP$")
	     (replace-match "<P>"))
	    ((looking-at "\\.LP")
	     (kill-line 1)
	     (throw 'exit ()))
	    ((looking-at "<")
	     (throw 'exit ()))
	    ((error "convert-one-ip-block-numbered")))))
  (insert "</OL>\n"))

(defun convert-one-ip-block-definition (regexp)
  (insert "<DL>\n")
  (catch 'exit
    (while (re-search-forward "^\\(\\.IP\\|\\.LP\\|<H\\|<P\\)")
      (beginning-of-line)
      (cond ((looking-at regexp)
	     (replace-match "<DT> \\1<DD>"))
	    ((looking-at "\\.IP$")
	     (replace-match "<P>"))
	    ((looking-at "\\.LP")
	     (kill-line 1)
	     (throw 'exit ()))
	    ((looking-at "<")
	     (throw 'exit ()))
	    ((error "convert-one-ip-block-definition")))))
  (insert "</DL>\n"))

(defun convert-one-ip-block ()
  (cond ((looking-at "\\.IP \\\\(bu 5$")
	 (convert-one-ip-block-unordered))
	((looking-at "\\.IP [0-9]\\. 5")
	 (replace-match "<OL>")
	 (convert-one-ip-block-numbered))
	((looking-at "\\.IP [^ ]+ 10")
	 (convert-one-ip-block-definition "\\.IP \\([^ ]+\\) 10"))
	((looking-at "\\.IP \\[[0-9]+\\] 5")
	 (convert-one-ip-block-definition "\\.IP \\[\\([0-9]+\\)\\] 5"))
	((error "convert-one-ip-block"))))

(defun convert-ip ()
  (while (re-search-forward "^\\.IP" () t)
    (beginning-of-line)
    (convert-one-ip-block)))

(defun convert-one-rs-block ()
  (kill-line 1)
  (insert "<UL>\n")
  (catch 'exit
    (while (re-search-forward "^\\.\\(IP\\|RE\\)")
      (beginning-of-line)
      (cond ((looking-at "\\.RE")
	     (throw 'exit ()))
	    ((looking-at "\\.IP \\\\- 5")
	     (kill-line 1)
	     (insert "<LI> "))
	    ((error "convert-one-rs-block")))))
  (kill-line 1)
  (insert "</UL>\n"))

(defun convert-rs ()
  (while (re-search-forward "^\\.RS" () t)
    (beginning-of-line)
    (convert-one-rs-block)))

(defun convert-special-a (from to)
  (goto-char (point-min))
  (re-search-forward from)
  (replace-match to))

(defun convert-special ()
  (re-search-forward "^\\.RS\n\\.LP")
  (replace-match ".LP")
  (re-search-forward "^\\.RE\n")
  (replace-match ""))
  
(defun convert-map (fn l)
  (while l
    (apply fn (car l))
    (setq l (cdr l))))

(defun convert-special-b (l)
  (convert-map (function (lambda (from to)
			(re-search-forward from)
			(replace-match to t t)))
	       l))

(setq ds-1 '("^\\.DS" "<PRE><CODE>"))
(setq de-1 '("^\\.DE" "</CODE></PRE>"))
(setq ds-list-1 (list ds-1 de-1))

(setq ds-2 '("^\\.Ds 0" "<PRE><CODE>"))
(setq de-2 '("^\\.De" "</CODE></PRE>"))
(setq ds-list-2 (list ds-2 de-2))

(defun convert-special-c (l)
  (convert-map (function (lambda (from to by)
			   (re-search-forward from)
			   (beginning-of-line)
			   (let ((start (point)))
			     (delete-region start (re-search-forward to))
			     (insert by))))
	       l))

(defun convert-ds ()
  (convert-special-b ds-list-1)
  (convert-special-c `(("^\\.DS" "^\\.DE" "<PRE><CODE>\n\tU<I>d</I>\t\t(e.g. U0 U1 U2 U3 ...)\n</CODE></PRE>")
		       ("^\\.DS" "^\\.DE" 
			"<PRE><CODE>\n\t<I>name</I>_U<I>d</I>\t(e.g. FOO_U0  BAR_U0  FOO_U1  BAR_U1  ...)\n</CODE></PRE>")
		       ("^\\.DS" "^\\.DE" "<PRE><CODE>\n\tFOO_R12345678_U23\n</CODE></PRE>")
		       ("^\\.Ds 0" "^\\.De" ,(concat "<PRE><CODE>\n"
						     "SetSelectionOwner(selection=PRIMARY, owner=Window, time=timestamp)\n"
						     "owner = GetSelectionOwner(selection=PRIMARY)\n"
						     "if (owner != Window) Failure\n"
						     "</CODE></PRE>"))
		       ("^\\.Ds 0" "^\\.De" ,(concat "<PRE><CODE>\n"
						     "win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), xsh.x, xsh.y,\n"
						     "\txsh.width, xsh.height, bw, bd, bg);\n"
						     "</CODE></PRE>"))
		       ("^\\.Ds 0" "^\\.De" ,(concat "<PRE><CODE>\n"
						     "win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), xsh.x, xsh.y,\n"
						     "\txsh.width, xsh.height, bw, bd, bg);\n"
						     "</CODE></PRE>"))
		       ("^\\.Ds" "^\\.De"
			,(concat "<MATH>width = base_width + (i &#215; width_inc)<BR>height = base_height + (j &#215; height_inc)</MATH>"))))
  (convert-special-b (append ds-list-1 ds-list-1 ds-list-1
			     ds-list-1 ds-list-1))             ;; lower-case
  (convert-special-c `(("^\\.DS" "^\\.DE" ,(concat "<OL>\n"
						   "<LI> RGB value/RGB intensity level pairs\n"
						   "<LI> RGB intensity ramp\n"
						   "</OL>"))
		       ("^\\.DS" "\\.DE" "<ADDRESS><A HREF=\"mailto:xregistry@x.org\">xregistry@x.org</A></ADDRESS>")
		       ("^\\.DS" "\\.DE" "<ADDRESS>X Consortium<BR>\n1 Memorial Dr<BR>\nCambridge MA 02142-1301<BR>\nUSA<BR>\n</ADDRESS>"))))


(defun convert-fs ()
  (goto-char (point-max))
  (insert "<HR>\n")
  (let ((fn-insertion-point (point-marker))
	(n 1))
    (goto-char (point-min))
    (while (re-search-forward "^\\.FS" () t)
      (beginning-of-line)
      (convert-one-fs))))

(defun convert-one-fs ()
  (kill-line 1)
  (let ((number (number-to-string n)))
    (setq n (1+ n))
    (insert "<A HREF=\"footnotes.html#f-" number "\"><SUP>" number "</SUP></A>")
    (let* ((start (point))
	   (end (re-search-forward "^\\.FE"))
	   (note (buffer-substring start (- end 3))))
      (delete-region start end)
      (goto-char fn-insertion-point)
      (insert "<FN><P>\n<A NAME=\"f-" number "\">" number ".</A> " note "</FN>\n\n")
      (setq fn-insertion-point (point-marker))
      (goto-char start))))

(defun convert-pn ()
  (while (re-search-forward "^\\.PN[ \t]+\\(.*\\)" () t)
    (replace-match "<B>\\1</B>")))

(defun convert-qp ()
  (while (re-search-forward "^\\.QP" () t)
    (beginning-of-line)
    (convert-one-qp)))

(defun convert-one-qp ()
  (kill-line 1)
  (insert "<BLOCKQUOTE>\n")
  (re-search-forward "^\\.LP")
  (replace-match "</BLOCKQUOTE>"))

(defun convert-cleanup-in-re ()
  (while (re-search-forward "^\\.\\(IN\\|RE\\)" () t)
    (beginning-of-line)
    (kill-line 1)))

(defun convert-one-bp ()
  (kill-line 1)
  (insert "<UL>\n<LI> ")
  (catch 'exit
    (while (re-search-forward "^\\(\\.bP\\|\\.LP\\|<H\\|<P\\|\\.IP\\)" () t)
      (beginning-of-line)
      (cond ((looking-at "\\.bP\n")
	     (replace-match "<LI> "))
	    ((looking-at "\\.IP$")
	     (replace-match "<P>"))
	    ((looking-at "\\.LP")
	     (kill-line 1)
	     (throw 'exit ()))
	    ((looking-at "<")
	     (throw 'exit ()))
	    ((error "convert-one-bp")))))
  (insert "</UL>\n"))

(defun convert-bp ()
  (while (re-search-forward "^\\.bP" () t)
    (beginning-of-line)
    (convert-one-bp)))

(defun convert-special-d (l)
  (convert-map (function (lambda (from into)
			   (goto-char (point-min))
			   (while (search-forward from () t)
			     (replace-match into))))
	       l))

(defun convert-special-e (l)
  (convert-map (function (lambda (from into then into-then)
			   (goto-char (point-min))
			   (while (re-search-forward from () t)
			     (replace-match into)
			     (re-search-forward then)
			     (replace-match into-then))))
	       l))

(defun convert-special-2 ()
  (convert-special-d '(("\\(em" "-") ("\\%" "") ("\\(->" "->") ("\\-" "-") ("\\^" "")
		       ("\\*(dA" "<->") ("\\|" "")))
  (convert-special-e '(("\\\\fI" "<I>" "\\\\fP" "</I>")
		       ("\\\\fB" "<B>" "\\\\fP" "</B>")
		       ("\\\\\\*Q" "&quot;" "\\\\\\*U" "&quot;")
		       ("^\\.I$" "<I>" "^\\.R$" "</I>"))))

(defun convert-one-sm ()
  (kill-line 1)
  (insert "<TABLE>\n<CAPTION>")
  (end-of-line)
  (insert "</CAPTION>")
  (let ((start (point))
	(end (copy-marker
	      (catch 'exit
		(while (re-search-forward "^\\(<P>\\|\\.br\\|\\.eM\\)" () t)
		  (beginning-of-line)
		  (cond ((looking-at "\\(<P>\\|\\.br\\)\n")
			 (replace-match "<TR><TD ALIGN=right>"))
			((looking-at "\\.eM")
			 (replace-match "</TABLE>")
			 (throw 'exit (point)))
			((error "convert-sm: internal error"))))))))
    (goto-char start)
    (while (search-forward ":" end t)
      (replace-match " :\t<TD>"))))

(defun convert-sm ()
  (while (re-search-forward "^\\.sM" () t)
    (beginning-of-line)
    (convert-one-sm)))

(defun convert-one-table (num-columns caption header &rest table)
  (re-search-forward "^\\.br$")
  (beginning-of-line)
  (let ((start (point))
	(end (copy-marker (re-search-forward "^\\.TE")))
	(ncol (number-to-string num-columns))
	(rule (concat "<TR><TH COLSPAN=" (number-to-string num-columns) "><HR>\n"))
	start-2 end-2)
    (delete-region start end)
    (insert "<TABLE>\n")
    (if caption
	(insert "<CAPTION>" caption "</CAPTION>\n"))
    (insert rule "<TR><TH ALIGN=left>" header "\n" rule)
    (setq start-2 (point-marker))
    (goto-char start)
    (while (search-forward "\t" start-2 t)
      (replace-match "\t<TH ALIGN=left>"))
    (goto-char start-2)
    (mapcar (function (lambda (s) (insert "<TR><TD>" s "\n"))) table)
    (setq end-2 (point-marker))
    (goto-char start-2)
    (while (search-forward "\t" end-2 t)
      (replace-match "\t<TD>"))
    (goto-char end-2)
    (insert rule "</TABLE>\n")))

(defun convert-tables ()
  (convert-one-table 3 () "Space	Briefly	Examples"
		     "Property name	Name	WM_HINTS, WM_NAME, RGB_BEST_MAP, ..."
		     "Property type	Type	WM_HINTS, CURSOR, RGB_COLOR_MAP, ..."
		     "Selection name	Selection	PRIMARY, SECONDARY, CLIPBOARD"
		     "Selection target	Target	FILE_NAME, POSTSCRIPT, PIXMAP, ..."
		     "Font property		QUAD_WIDTH, POINT_SIZE, ..."
		     "<B>ClientMessage</B> type		WM_SAVE_YOURSELF, _DEC_SAVE_EDITS, ...")
  (convert-one-table  3 () "Name Discriminated By	Form	Example"
		     "screen number	<I>name</I>_S<I>d</I>	WM_COMMS_S2"
		     "X resource	<I>name</I>_R<I>x</I>	GROUP_LEADER_R1234ABCD")
  (convert-one-table 3 () "Atom	Type 	Data Received"
		     "ADOBE_PORTABLE_DOCUMENT_FORMAT	STRING	[1]"
		     "APPLE_PICT	APPLE_PICT	[2]"
		     "BACKGROUND	PIXEL	A list of pixel values"
		     "BITMAP	BITMAP	A list of bitmap IDs"
		     "CHARACTER_POSITION	SPAN	The start and end of the selection in bytes"
		     "CLASS	TEXT	(see section 4.1.2.5)"
		     "CLIENT_WINDOW	WINDOW	Any top-level window owned by the selection owner"
		     "COLORMAP	COLORMAP	A list of colormap IDs"
		     "COLUMN_NUMBER	SPAN	The start and end column numbers"
		     "COMPOUND_TEXT	COMPOUND_TEXT	Compound Text"
		     "DELETE	NULL	(see section 2.6.3.1)"
		     "DRAWABLE	DRAWABLE	A list of drawable IDs"
		     "ENCAPSULATED_POSTSCRIPT	STRING	[3], Appendix H<A HREF=\"footnotes.html#f-5\"><SUP>5</SUP></A>"
		     "ENCAPSULATED_POSTSCRIPT_INTERCHANGE	STRING	[3], Appendix H"
		     "FILE_NAME	TEXT	The full path name of a file"
		     "FOREGROUND	PIXEL	A list of pixel values"
		     "HOST_NAME	TEXT	(see section 4.1.2.9)"
		     "INSERT_PROPERTY	NULL	(see section 2.6.3.3)"
		     "INSERT_SELECTION	NULL	(see section 2.6.3.2)"
		     "LENGTH	INTEGER	The number of bytes in the selection<A HREF=\"footnotes.html#f-6\"><SUP>6</SUP></A>"
		     "LINE_NUMBER	SPAN	The start and end line numbers"
		     "LIST_LENGTH	INTEGER	The number of disjoint parts of the selection"
		     "MODULE	TEXT	The name of the selected procedure"
		     "MULTIPLE	ATOM_PAIR	(see the discussion that follows)"
		     "NAME	TEXT	(see section 4.1.2.1)"
		     "ODIF	TEXT	ISO Office Document Interchange Format"
		     "OWNER_OS	TEXT	The operating system of the owner client"
		     "PIXMAP	PIXMAP<A HREF=\"footnotes.html#f-7\"><SUP>7</SUP></A>	A list of pixmap IDs"
		     "POSTSCRIPT	STRING	[3]"
		     "PROCEDURE	TEXT	The name of the selected procedure"
		     "PROCESS	INTEGER, TEXT	The process ID of the owner"
		     "STRING	STRING	ISO Latin-1 (+TAB+NEWLINE) text"
		     "TARGETS	ATOM	A list of valid target atoms"
		     "TASK	INTEGER, TEXT	The task ID of the owner"
		     "TEXT	TEXT	The text in the owner's choice of encoding"
		     "TIMESTAMP	INTEGER	The timestamp used to acquire the selection"
		     "USER	TEXT	The name of the user running the owner")
  (re-search-forward "^\\.nr \\*")
  (delete-region (point) (re-search-forward "7.*/A>$"))
  (convert-one-table 3 () "Type Atom	Format	Separator"
		     "APPLE_PICT	8	Self-sizing"
		     "ATOM	32	Fixed-size"
		     "ATOM_PAIR	32	Fixed-size"
		     "BITMAP	32	Fixed-size"
		     "C_STRING	8	Zero"
		     "COLORMAP	32	Fixed-size"
		     "COMPOUND_TEXT	8	Zero"
		     "DRAWABLE	32	Fixed-size"
		     "INCR	32	Fixed-size"
		     "INTEGER	32	Fixed-size"
		     "PIXEL	32	Fixed-size"
		     "PIXMAP	32	Fixed-size"
		     "SPAN	32	Fixed-size"
		     "STRING	8	Zero"
		     "WINDOW	32	Fixed-size")
  (convert-one-table 2 () "Argument	Value"
		     "destination:	the root window of screen 0, or the root manager is managing a screen-specific resource"
		     "propagate:	False"
		     "event-mask:	<B>StructureNotify</B>"
		     "event:	<B>ClientMessage</B>"
		     " type:	MANAGER"
		     " format:	32"
		     " data[0]:\\**	timestamp"
		     " data[1]:	manager selection atom"
		     " data[2]:	the window owning the selection"
		     " data[3]:	manager-selection-specific data"
		     " data[4]:	manager-selection-specific data")
  (kill-line 1)
  (convert-one-table 3 () "Field	Type	Comments"
		     "flags	CARD32	(see the next table)"
		     "pad	4*CARD32	For backwards compatibility"
		     "max_width	INT32"
		     "max_height	INT32"
		     "width_inc	INT32"
		     "height_inc	INT32"
		     "max_aspect	(INT32,INT32)"
		     "base_width	INT32	If missing, assume min_width"
		     "base_height	INT32	If missing, assume min_height"
		     "win_gravity	If missing, assume <B>NorthWest</B>")
  (convert-one-table 3 () "Name	Value	Field"
		     "<B>USPosition</B>	1	User-specified x, y"
		     "<B>USSize</B>	2	User-specified width, height"
		     "<B>PPosition</B>	4	Program-specified position"
		     "<B>PSize</B>T}	8	Program-specified size"
		     "<B>PMinSize</B>	16	Program-specified minimum size"
		     "<B>PMaxSize</B>	32	Program-specified maximum size"
		     "<B>PResizeInc</B>	64	Program-specified resize increments"
		     "<B>PAspect</B>	128	Program-specified min and max aspect ratios"
		     "<B>PBaseSize</B>	256	Program-specified base size"
		     "<B>PWinGravity</B>	512	Program-specified window gravity")
  (convert-one-table 3 () "Field	Type	Comments"
		     "flags	CARD32	(see the next table)"
		     "input	CARD32	The client's input model"
		     "initial_state	CARD32	The state when first mapped"
		     "icon_pixmap	PIXMAP	The pixmap for the icon image"
		     "icon_window	WINDOW	The window for the icon image"
		     "icon_x	INT32	The icon location"
		     "icon_y	INT32"
		     "icon_mask	PIXMAP	The mask for the icon shape")
  (convert-one-table 3 () "Name	Value	Field"
		     "<B>InputHint</B>	1	input"
		     "<B>StateHint</B>	2	initial_state"
		     "<B>IconPixmapHint</B>	4	icon_pixmap"
		     "<B>IconWindowHint</B>	8	icon_window"
		     "<B>IconPositionHint</B>	16	icon_x & icon_y"
		     "<B>IconMaskHint</B>	32	icon_mask"
		     "<B>WindowGroupHint</B>	64	window_group"
		     "<B>MessageHint</B>	128	(this bit is obsolete)"
		     "<B>UrgencyHint</B>	256	urgency")
  (convert-one-table 3 () "State	Value	Comments"
		     "<B>NormalState</B>	1	The window is visible"
		     "<B>IconicState</B>	3	The icon is visible")
  (convert-one-table 3 () "Protocol	Section	Purpose"
		     "WM_TAKE_FOCUS	4.1.7	Assignment of input focus"
		     "WM_SAVE_YOURSELF	Appendix C	Save client state request (deprecated)"
		     "WM_DELETE_WINDOW	4.2.8.1	Request to delete top-level window")
  (convert-one-table 3 () "Field	Type	Comments"
		     "state	CARD32	(see the next table)"
		     "icon	WINDOW	ID of icon window")
  (convert-one-table 2 () "State	Value"
		     "<B>WithdrawnState</B>	0"
		     "<B>NormalState</B>	1"
		     "<B>IconicState</B>	3")
  (convert-one-table 3 () "Field	Type	Comments"
		     "max_width	CARD32"
		     "max_height	CARD32"
		     "width_inc	CARD32"
		     "height_inc	CARD32")
  (convert-one-table 2 () "Argument	Value"
		     "destination:	The root"
		     "propagate:	<B>False</B>"
		     "event-mask:	<B>( SubstructureRedirect|SubstructureNotify )</B>"
		     "event: an <B>UnmapNotify</B> with:	"
		     "event:	The root"
		     "window:	The window itself"
		     "from-configure:	<B>False</B>")
  (convert-one-table 2 () "Argument	Value"
		     "destination:	The root"
		     "propagate:	<B>False</B>"
		     "event-mask:	<B>( SubstructureRedirect|SubstructureNotify )</B>"
		     "event: a <B>ConfigureRequest </B> with:	"
		     " event:	The root"
		     " window:	The window itself"
		     " ...	Other parameters from the <B>ConfigureWindow</B> request")
  (convert-one-table 2 () "Attribute	Private to Client"
		     "Background pixmap	Yes"
		     "Background pixel	Yes"
		     "Border pixmap	Yes"
		     "Border pixel	Yes"
		     "Bit gravity	Yes"
		     "Backing-store hint	Yes"
		     "Save-under hint	No"
		     "Event mask	No"
		     "Do-not-propagate mask	Yes"
		     "Override-redirect flag	No"
		     "Colormap	Yes"
		     "Cursor	Yes")
  (convert-one-table 3 () "Input Model	Input Field	WM_TAKE_FOCUS"
		     "No Input	<B>False</B>	Absent"
		     "Passive	<B>True</B>	Absent"
		     "Locally Active	<B>True</B>	Present"
		     "Globally Active	<B>False</B>	Present")
  (convert-one-table 2 () "Argument	Value"
		     "destination:	the root window of the screen on which the colormap is being installed"
		     "propagate:	<B>False</B>"
		     "event-mask:	<B>ColormapChange</B>"
		     "event: a <B>ClientMessage</B> with:	"
		     " window:	the root window, as above"
		     " type:	WM_COLORMAP_NOTIFY"
		     " format:	32"
		     " data[0]:	the timestamp of the event that caused the client to start or stop installing colormaps"
		     " data[1]:	1 if the client is starting colormap installation, 0 if the client is finished with colormap installation"
		     " data[2]:	reserved, must be zero"
		     " data[3]:	reserved, must be zero"
		     " data[4]:	reserved, must be zero")
  (re-search-forward "^\\.br$")
  (kill-line 2)
  (convert-one-table 2 () "Argument	Value"
		     "destination:	The client's window"
		     "propagate:	<B>False</B>"
		     "event-mask:	<B>StructureNotify</B>")
  (convert-one-table 2 () "Argument	Value"
		     "destination:	The client's window"
		     "propagate:	<B>False</B>"
		     "event-mask:	() empty"
		     "event:	As specified by the protocol")
  (convert-one-table 3 () "Atom	Type	Data Received"
		     "VERSION	INTEGER	Two integers, which are the major and minor release numbers (respectively) of the ICCCM with which the window manager complies.  For this version of the ICCCM, the numbers are 2 and 0.\\**")
  (kill-line 1)
  (convert-one-table 4 () "Name	Type	Format	See Section"
		     "WM_CLASS	STRING	8	4.1.2.5"
		     "WM_CLIENT_MACHINE	TEXT	\&	4.1.2.9"
		     "WM_COLORMAP_WINDOWS	WINDOW	32	4.1.2.8"
		     "WM_HINTS	WM_HINTS	32	4.1.2.4"
		     "WM_ICON_NAME	TEXT		4.1.2.2"
		     "WM_ICON_SIZE	WM_ICON_SIZE	32	4.1.3.2"
		     "WM_NAME	TEXT		4.1.2.1"
		     "WM_NORMAL_HINTS	WM_SIZE_HINTS	32	4.1.2.3"
		     "WM_PROTOCOLS	ATOM	32	4.1.2.7"
		     "WM_STATE	WM_STATE	32	4.1.3.1"
		     "WM_TRANSIENT_FOR	WINDOW	32	4.1.2.6")
  (convert-one-table 3 () "Field	Type	Comments"
		     "colormap	COLORMAP	ID of the colormap described"
		     "red_max	CARD32	Values for pixel calculations"
		     "red_mult	CARD32"
		     "blue_max	CARD32"
		     "blue_mult	CARD32"
		     "base_pixel	CARD32"
		     "visual_id	VISUALID	Visual to which colormap belongs"
		     "kill_id	CARD32	ID for destroying the resources")
  (convert-one-table 3 "XDCCC_LINEAR_RGB_MATRICES property contents" "Field	Type	Comments"
		     "<I>M<SUB>0,0</SUB></I>	INT32	Interpreted as a fixed-point number <I>-16 &lt;= x < 16</I>"
		     "<I>M<SUB>0,1</SUB></I>	INT32	"
		     "...		"
		     "<I>M<SUB>3,3</SUB></I>	INT32	"
		     "<I>M<SUP>-1</SUP><SUB>0,0</SUB></I>	INT32	"
		     "<I>M<SUP>-1</SUP><SUB>0,1</SUB></I>	INT32	"
		     "...		"
		     "<I>M<SUP>-1</SUP><SUB>3,3</SUB></I>	INT32	")
  (convert-one-table 3 "XDCCC_LINEAR_RGB_CORRECTION Property Contents for Type 0 Correction" "Field	Type	Comments"
		     "VisualID0	CARD	Most-significant portion of VisualID"
		     "VisualID1	CARD	Exists if and only if the property format is 8"
		     "VisualID2	CARD	Exists if and only if the property format is 8"
		     "VisualID3	CARD	Least-significant portion, exists if and only if the property format is 8 or 16"
		     "type	CARD	0 for this type of correction"
		     "count	CARD	Number of tables following (either 1 or 3)"
		     "length	CARD	Number of pairs &endash; 1 following in this table"
		     "value	CARD	X Protocol RGB value"
		     "intensity	CARD	Interpret as a number <I>0 &lt;= intensity &lt;= 1</I>"
		     "...	...	Total of <I>length+1</I> pairs of value/intensity values"
		     "lengthg	CARD	Number of pairs &endash; 1 following in this table (if and only if <I>count</I> is 3)"
		     "value	CARD	X Protocol RGB value"
		     "intensity	CARD	Interpret as a number <I>0 &lt;= intensity &lt;= 1</I>"
		     "...	...	Total of <I>lengthg+1</I> pairs of value/intensity values"
		     "lengthb	CARD	Number of pairs &endash; 1 following in this table (if and only if <I>count</I> is 3)"
		     "value	CARD	X Protocol RGB value"
		     "intensity	CARD	Interpret as a number <I>0 &lt;= intensity &lt;= 1</I>"
		     "...	...	Total of <I>lengthb+1</I> pairs of value/intensity values")
  (convert-one-table 3 "XDCCC_LINEAR_RGB_CORRECTION Property Contents for Type 1 Correction" "Field	Type	Comments"
		     "VisualID0	CARD	Most-significant portion of VisualID"
		     "VisualID1	CARD	Exists if and only if the property format is 8"
		     "VisualID2	CARD	Exists if and only if the property format is 8"
		     "VisualID3	CARD	Least-significant portion, exists if and only if the property format is 8 or 16"
		     "type	CARD	1 for this type of correction"
		     "count	CARD	Number of tables following (either 1 or 3)"
		     "length	CARD	Number of elements &endash; 1 following in this table"
		     "intensity	CARD	Interpret as a number <I>0 &lt;= intensity &lt;= 1</I>"
		     "...	...	Total of <I>length+1</I> intensity elements"
		     "lengthg	CARD	Number of elements &endash; 1 following in this table (if and only if <I>count</I> is 3)"
		     "intensity	CARD	Interpret as a number <I>0 &lt;= intensity &lt;= 1</I>"
		     "...	...	Total of <I>lengthg+1</I> intensity elements"
		     "lengthb	CARD	Number of elements &endash; 1 following in this table (if and only if <I>count</I> is 3)"
		     "intensity	CARD	Interpret as a number <I>0 &lt;= intensity &lt;= 1</I>"
		     "...	...	Total of <I>lengthb+1</I> intensity elements")
  (convert-one-table 4 () "Name	Type	Format	See Section"
		     "WM_CLIENT_MACHINE	TEXT		4.1.2.9"
		     "WM_COMMAND	TEXT		C.1.1"
		     "WM_STATE	WM_STATE	32	4.1.3.1"))

(defun convert-move-footnotes ()
  (while (search-forward "\\**" () t)
    (forward-char -3)
    (convert-move-one-footnote)))

(defun convert-move-one-footnote ()
  (let ((start (point))
	(note (progn
		(re-search-forward "^<A HREF=\"footnotes.html#f-[0-9]+\">.*</A>\n")
		(bsub 0))))
    (replace-match "")
    (goto-char start)
    (delete-char 3)
    (insert note)))
	      
(defun convert-math ()
  (convert-special-c '(("^\\.EQ$" "^\\.EN$" "")))
  (convert-special-b '(("@3 times 3@" "<I>3 &#215; 3</I>")
		       ("@M@" "<I>M</I>")
		       ("@M sup -1@" "<I>M<SUP>-1</SUP></I>")
		       ("@RGB sub intensity@" "<I>RGB<SUB>intensity</SUB></I>")))
  (convert-special-c '(("^\\.EQ C$" "^\\.EN$" "<P ALIGN=center><I>RGB<SUB>intensity</SUB> = M &#215; XYZ</I><BR>")
		       ("^\\.EQ C$" "^\\.EN$" "<I>XYZ = M<SUP>-1</SUP> &#215; RGB<SUB>intensity</SUB></I>")))
  (convert-special-b '(("@2 sup 27@" "<I>2<SUP>27</SUP></I>")
		       ("@-16@" "<I>-16</I>")
		       ("@16 - epsilon@" "<I>16 - &epsilon;</I>")
		       ("@epsilon ~ = ~ 2 sup -27@" "<I>&epsilon; = 2 <SUP>-27</SUP></I>")))
  (convert-special-c `(("^\\.EQ C$" "^\\.EN$" ,(concat "<I><TABLE>\n"
						       "<TR><TD ROWSPAN=3 VALIGN=center>RGB<SUB>value</SUB> =	<TD>Property Value &#215; 65535\n"
						       "<TR>						<TD><HR>\n"
						       "<TR>						<TD ALIGN=center>255\n"
						       "</TABLE></I>\n"))
		       ("^\\.EQ C$" "^\\.EN$" ,(concat "<I><TABLE>\n"
							"<TR><TD ROWSPAN=3 VALIGN=center>RGB<SUB>value</SUB> =	<TD>Array Index &#215; 65535\n"
							"<TR>						<TD><HR>\n"
							"<TR>						<TD ALIGN=center>Array Size - 1\n"
							"</TABLE></I>\n"))
		       ("^\\.EQ$" "^\\.EN$" ""))))

(defun convert-final-cleanup ()
  (convert-special-b '(("Appendix A.$" "<A HREF=\"sec-A.html#s-A\">Appendix A</A>.")
		       ("Appendix B,$" "<A HREF=\"sec-B.html#s-B\">Appendix B</A>,")
		       ("^\\.sp\n" "")
		       ("^\\.nr \\*\n" "")
		       (">Appendix C" "><A HREF=\"sec-C.html#s-C\">Appendix C</A>")
		       ("^\\.\\\\\".*\n\\.\\\\.*\n\\.br" "")
		       ("^<UL>\n.*\n\\.nr H1 0\n\\.af H1 A\n\\.cT.*no\n\n</UL>\n" "")
		       ("^<LI> .cT \"Appendix B\" no\n\n" "")
		       ("^<LI> .cT \"Appendix C\" no\n\n" "")
		       ("^\\.\\\\\" Finish up!\n\\.YZ 3\n" "")
		       ("\\\\s-2\\\\uth\\\\d\\\\s0" "<SUP>th</SUP>"))))


(defun convert-hyperlinks ()
  (while (re-search-forward "\\(section \\)\\(\\([0-8A-C]\\)\\(\\.[0-9]+\\)+\\)\\|\\(\\([0-8A-C]\\)\\(\\.[0-9]+\\)+\\)" () t)
    (if (save-match-data (beginning-of-line) (looking-at "<H[0-9]>"))
	(forward-line 1)
	(let* ((section (if (match-beginning 1) (bsub 1)))
	       (index (if section 2 5)))
	  (replace-match (concat "<A HREF=\"sec-" (bsub (1+ index)) ".html#s-" (bsub index) "\">" section (bsub index) "</A>") t t)))))

(defun convert-next (v)
  (cond ((>= v ?C) ()) ((= v ?8) ?A) ((1+ v))))

(defun convert-prev (v)
  (cond ((<= v ?1) ()) ((= v ?A) ?8) ((1- v))))

(defun convert-explode-one ()
  (let* ((start (point))
	 (end (progn (forward-char 1) (re-search-forward "^<H[1R]>") (beginning-of-line) (point)))
	 (file (buffer-substring start end))
	 (section (progn (goto-char start) (re-search-forward "NAME=\"s-\\([0-8A-C]\\)\">.*</A> *\\([^ ][^<]+\\)</H1>") (bsub 1)))
	 (section-name (bsub 2)))
    (delete-region start end)
    (save-excursion
      (set-buffer (find-file-noselect (concat "sec-" section ".html")))
      (erase-buffer)
      (insert "<HTML>\n<HEAD>\n<TITLE>ICCCM - " section-name "</TITLE>\n</HEAD>\n\n<BODY>\n" file "<HR>\n")
      (let* ((s (string-to-char section))
	     (next (convert-next s))
	     (prev (convert-prev s)))
	(if prev (insert "<A HREF=\"sec-" prev ".html\"><IMG SRC=\"/~tronche/images/left.gif\" WIDTH=31 HEIGHT=31 ALT=\"<\"></A>"))
	(insert "<A HREF=\"manual.html\"><IMG SRC=\"/~tronche/images/up.gif\" WIDTH=31 HEIGHT=31 ALT=\"^\"></A>")
	(if next (insert "<A HREF=\"sec-" next ".html\"><IMG SRC=\"/~tronche/images/right.gif\" WIDTH=31 HEIGHT=31 ALT=\">\"></A>")))
      (insert "\n<P><ADDRESS><A HREF=\"/~tronche/\">Christophe Tronche</A></ADDRESS>\n</BODY>\n</HTML>\n"))))

(defun convert-explode ()
  (interactive)
  (while (looking-at "<H1>")
    (convert-explode-one))
  (kill-line)
  (insert "<HTML>\n<HEAD>\n<TITLE>ICCCM - Footnotes</TITLE>\n</HEAD>\n\n"
	  "<BODY>\n<H1>Footnotes</H1>\n")
  (goto-char (point-min))
  (insert "</BODY>\n</HTML>\n")
  (set-visited-file-name "footnotes.html"))
  
    

;;"[1] 14.1.12 2.0 14.3.1 1.0 1.1 1.0 2.0 table captions

;;grep '^<H[0-9]>' icccm.html | sed -e 's/^<H[0-9]>/<LI>/' -e 's/NAME="\(s-\(.\)[^"]*\)">/HREF="sec-\2.html#\1">/' -e 's;</A>;;' -e 's;</H;</A></H;' > toc.html
