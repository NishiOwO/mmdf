/*
 *  $Id: ml_send.h,v 1.1 1998/05/25 20:11:10 krueger Exp $
 */

/* send a piece of mail, using a simple interface to submit */

/*  Basic sequence is:
 *
 *          ml_init (YES, NO, "My Name", "The Subject");
 *          ml_adr ("destination address 1");
 *          ml_adr ("destination address 2");
 *          ...
 *          ml_aend ();
 *          ml_tinit ();
 *          ml_txt ("Some opening text");
 *          ml_txt ("maybe some more text");
 *          ml_file (file-stream-descriptor-of-file-to-include);
 *          if (ml_end (OK)) != OK)
 *          {   error-handling code }
 *
 *  Arguments that are to be defaulted should be zero.
 *
 *  ml_init's arguments specify a) whether return-mail (to the sender
 *  should be allowed, b) whether a Sender field should be used to
 *  specify the correct sender (contingent on next argument), c) text
 *  for the From field, and d) text for the Subject field.  If (b) is
 *  NO, then (c)'s text will be followed by the correct sender
 *  information.
 *
 *  ml_to and ml_cc are used to switch between To and CC addresses.
 *  Normally, only To addresses are used and, for this, no ml_to call is
 *  needed.
 *
 *  An "address" is whatever is valid for your system, as if you were
 *  typing it to the mail command.
 *
 *  You may freely mix ml_txt and ml_file calls.  They just append text
 *  to the message.  The text must contain its own newlines.
 *
 *  State diagram: ml_state:
 *       ML_FRESH --> submit running --> ML_MSG  --> ready for new message -->
 *       ML_HEADER --> reading header --> ML_TEXT --> message sent --> ML_MSG
 *
 */

extern int ml_init ();
extern int ml_to ();
extern int ml_cc ();
extern int ml_adr ();
extern int ml_aend ();
extern int ml_tinit ();
extern int ml_sndhdr ();
extern int ml_file ();
extern int ml_txt();
extern int ml_end ();
extern int ml_1adr ();
extern void ml_sender ();
