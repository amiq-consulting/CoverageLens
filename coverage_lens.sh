#!/bin/bash
make run ARGS="$*" 


if [ -f "amiq_recipient_list" ]
then
	
	# cat amiq_recipient_list;

	(
		echo "From: your_mail@here";
		echo "To: `cat amiq_recipient_list`";
		echo "Subject: Coverage Lens report"
		echo "Content-Type: text/html";
		echo "MIME-Version: 1.0";
		echo "";
		cat amiq_body.html;
	) | sendmail -t

	rm -f amiq_recipient_list amiq_body.html


fi
