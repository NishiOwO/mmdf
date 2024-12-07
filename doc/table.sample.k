        Host and Domain Table examples

        Steve Kille

        Febraury 1984

This text is meant to illustrate by example, the mechanism for
setting up host and domain tables.   It is assumed that the
manual pages tailor(5.mmdf) tables(5.mmdf) have been read if
not understood.   This sheet contains three example:
 1) A simple Arpa host with minimal tables
 2) A UK host, illustrating handling of local domain with more
        than two components, and basic use of the list channel
 3) A complex example, illustrating use of locmachine, and various
        other subtle points

WARNING:  Some of the material here is out of date.  Consult the samples
directory for other example tables.


1.  Basic ARPA host

The following tailor values are assumed in these examples:

LOCDOMAIN       = ARPA
LOCNAME         = Ivy


For reference in the following tables, the structure is based
on the following table, channel, and domain specifications.

# Only one domain entry needed.  The table maps to a file, and
# then MDMN specifies the entry.  Note that the flags entry is
# probably redundant, but must always be set to 01.

MTBL    "ARPA",file="arpa",show="ARPA Domain"
MDMN    "ARPA",show="ARPA Domain",table="ARPA"

# Channel parameters, with fields not pertinent to address lookup removed

MCHN  local    show="Local Channel"

MCHN  smtp  show="SMTP channel"


First the channel tables will be looked at.  When domains are
reformatted, either host or domain values are output.  The
selection of host names is related to current DCA and JNT
domain policy.  Domains that are 'allowed' are also host names.
In other cases, abbreviated names are used.


# Local channel table

# note - RHS = LOCNAME in all cases
# values for LHS include LOCNAME, and all full domain names
# Northing else should ever be needed
ivy.arpa:ivy
ivy:ivy

# TCP/SMTP hosts channel table


# Only official HOST names in table
alta-coma.arpa:3.1.0.50
yuma.arpa:6.1.0.1
bbncca.arpa:8.0.0.2
bbnccb.arpa:8.1.0.2
bbnccc.arpa:8.2.0.2
bbnccd.arpa:8.3.0.2
bbn-labs-admin.arpa:8.4.0.2
bbncck.arpa:8.5.0.2
.....


#  ARPA Domain table


#
# Here are all of the names from hosts.txt
# Official name first.  HOST  name (as used in channel table) on RHS
alta-coma:alta-coma.arpa
yuma:yuma.arpa
bbncca:bbncca.arpa
bbnccb:bbnccb.arpa
bbnccc:bbnccc.arpa
bbnccd:bbnccd.arpa
bbn-labs-admin:bbn-labs-admin.arpa
bbn-labs:bbn-labs-admin.arpa
bbnl:bbn-labs-admin.arpa
bbncck:bbncck.arpa
......




2.  Baisc UK Host

The following tailor values are assumed in these examples:

LOCDOMAIN       = Camford.AC.UK
LOCNAME         = CS


For reference in the following tables, the structure is based
on the following table, channel, and domain specifications.


#  Camford.AC.UK - always the most local domain first.  The table
#  maps to a file, and the domin spec points to the table
# This domain is for other hosts within the camford domain
# (e.g physics.camford.ac.uk)

MTBL    "Camford",file="camford",show="camford.ac.uk domain table"
MDMN    "Camford.AC.UK",show="Camford.AC.UK domain",table="Camford"

# Then The AC.UK domain for other .AC.UK domains

MTBL    "AC-UK",file="ac-uk",show="AC.UK Domain"
MDMN    "AC.UK",show="AC.UK Domain",table="AC-UK"


# then AC.UK domain, which uses the same table as AC.UK
# The settings of name=AC  and dmn=AC.UK will let
# foo.ac  be mapped to foo.ac.uk

MDMN    "AC",show="AC.UK Domain (AC)",table="AC-UK",name="AC",dmn="AC.UK"


# Channel parameters, with fields not pertinent to address lookup removed

MCHN  local    show="Local Channel"

MCHN  list    show="via List-Channel"

# in this case, the list channel will use the local channel
# table, rather than needing its own table.  If foo is a list,
# The alis file would have entries:
#       foo:foo-outbound@camford@list
#       foo-outbound: :include:/etc/alias/foo
# This would force foo to be deliverd to the list channel.
# The list is then expanded in background

MCHN  niserc   show="via Sercnet with NIFTP"

MCHN uucp  show="with UUCP"



First the channel tables will be looked at.  When domains are
reformatted, either host or domain values are output.  The
selection of host names is related to current JNT
domain policy.  Domains that are 'allowed' are also host names.
In other cases, abbreviated names are used.  (e.g. until the
NRS is introduced, hostnames are foo rather than foo.ac.uk).

NOTE on NRS ordering (Americans ignore this): Internal tables
all use RFC 819 ordering.  Mapping for reformatting output
should be selected.


# Local channel table

# note - RHS = LOCNAME in all cases
# values for LHS include LOCNAME, and all full domain names
cs.camford.ac.uk:cs
cs:cs




# Sercnet Table

# Note that RHS is mapped to a Transport address by the lower
# level (NIFTP)
#
#  As neither DCA nor JNT would like to see names with .AC.UK
# in, UK host names are not full domains.
abxa:abxa/mail
apca:apca/mail
baca:baca/mail
# Some other camford hosts are reached over sercnet
bio.camford:bio.camford/mail

# UUCP table

# This might be used for both remote and camford hosts
eagle:eagle
physics.camford:camphys



The rest of this sample description shows the structuring of
the domain tables.


#  Camford.AC.UK domain table


# This contains all *.camford.ac.uk
# with alternative anmes on LHS, and HOST names on RHS
# Official names first
# Ourselves - note lowercase in tables
cs:cs
comp-sci:cs
# And the rest
bio:bio.camford
biology:bio.camford
physics:physics.camford




# AC.UK Domain table


#
#    List of *.AC.UK
# Note that some are in different channels
abxa:abxa
apca:apca
baca:baca
eagle:eagle



3.  Complex Example

A real extract is given, as this is more straightforeward to
check!

The following tailor values are assumed in these examples:

LOCDOMAIN       = AC.UK
LOCNAME         = Ucl-Cs
LOCMACHINE      = 44a


This mechanism should still work if LOCMACHINE is not set.
For reference in the following tables, the structure is based
on the following table, channel, and domain specifications.


#  Ucl-Cs.AC.UK - always the most local domain first.  The table
#  maps to a file, and the domin spec points to the table

MTBL    "Ucl-Cs",file="ucl-cs",show="Ucl-Cs domain table"
MDMN    "Ucl-Cs.AC.UK",show="Ucl-Cs.AC.UK domain",table="Ucl-Cs"

# Then AC.UK

MTBL    "AC-UK",file="ac-uk",show="AC.UK Domain"
MDMN    "AC.UK",show="AC.UK Domain",table="AC-UK"

# then AC domain, which uses the same table as AC.UK
# The settings of name=AC  and dmn=AC.UK will let
# foo.ac  be mapped to foo.ac.uk

MDMN    "AC",show="AC.UK Domain (AC)",table="AC-UK",name="AC",dmn="AC.UK"

MTBL    "ARPA",file="arpa",show="ARPA Domain"
MDMN    "ARPA",show="ARPA Domain",table="ARPA"

MTBL    "MAILNET",file="mailnet",show="Mailnet Domain"
MDMN    "MAILNET",show="Mailnet Domain",table="MAILNET"

# The top level domain is entered by setting Name and dmn to ""

MTBL    "Top-Level",file="top",show="Top Level Domain"
MDMN    "Top-Level",show="Top level Domain",table="Top-Level",name="",dmn=""


# Channel parameters, with fields not pertinent to address lookup removed

MCHN  local    show="Local (44a)"

MCHN  list    show="via List-Channel"

MCHN  sring show="with SMTP"  (around local ring)

MCHN  satnet  show="via Satnet with SMTP",ldomain="ARPA"

MCHN  niserc   show="via Sercnet with NIFTP"



First the channel tables will be looked at.  When domains are
reformatted, either host or domain values are output.  The
selection of host names is related to current DCA and JNT
domain policy.  Domains that are 'allowed' are also host names.
In other cases, abbreviated names are used.


# Local channel table

# note - RHS = LOCNAME in all cases
# values for LHS include LOCNAME, and all full domain names
ucl-cs:ucl-cs
ucl-cs.ac.uk:ucl-cs
ucl-cs.arpa:ucl-cs
# These two ar only needed if LOCMACHINE is set
# The LHS should be LOCMACHINE and LOC-MACHINE+NAME+DOMAIN
44a:ucl-cs
44a.ucl-cs.ac.uk:ucl-cs


# List channel table

# This has only one entry, and is only needed if LOCMACHINE is
# set to give transparent routing between a number  of machines
# This enables lists on other machines to be referred to iin
# the same manner
# in this case, an alis file entry might be
#       foo:foo-outbound@44c-list
#       foo-outbound: :include: /etc/alias/foo@44c
# The @44c in the second line ensures that mail sent to foo-outbound
# works correctly
44a-list:44a-list


# Ring channel table (abbreviated)


# This is the table associated with the local ring
# The local references need NOT be removed, and so all machines
# can use the same table.  These names are not seen by
# users unless typed explicity.  RHS is transport address.
ucl-cs.arpa:128.16.9.3
44a:smtp44a
44b:smtp44b
44c:smtp44c
...
# References to other list channels are included so that non-local
# lists will be sent around the local net to the next machine
44a-list:smtp44a
44b-list:smtp44b
44c-list:smtp44c
...



# TCP/SMTP hosts channel table


# Note special reference to ourselves at the top of the table
# To protect against table update errors
# NOte that multiples of the RHS are included to do the
# reverse mapping correctly from all registered IP addresses
ucl-cs.arpa:128.16.9.3
ucl-cs.arpa:14.0.0.9
# This is needed for convenient loopback testing
# (e.g to use steve@ucl-cs@satnet  rather than steve@ucl-cs.arpa@satnet
ucl-cs:128.16.9.3
#
#    Arpa hosts are all full domain names.   RHS is familiar numbers
#  Note that no abbreviated names are in this table
alta-coma.arpa:3.1.0.50
yuma.arpa:6.1.0.1
bbncca.arpa:8.0.0.2
bbnccb.arpa:8.1.0.2
bbnccc.arpa:8.2.0.2
bbnccd.arpa:8.3.0.2
bbn-labs-admin.arpa:8.4.0.2
bbncck.arpa:8.5.0.2
.....



# Sercnet Table

# Note that RHS is mapped to a Transport address by the lower
# level (NIFTP)
#
#  As neither DCA nor JNT would like to see names with .AC.UK
# in, UK host names are not full domains.
abxa:abxa/mail
apca:apca/mail
baca:baca/mail
bdga:bdga/mail
bdgb:bdgb/mail
bedford:bedford/mail
bgxa:bgxa/mail
bhga:bhga/mail
bhgb:bhgb/mail
bhia:bhia/mail
bhvs:bhvs/mail
bihh1:bihh1/mail
bihh2:bihh2/mail
bihh3:bihh3/mail
# UK users - note domain ordering!!
# US users - ignore this entry
hcig.nott:nott.hcig/mail
nsga:nsga/mail





The rest of this sample description shows the structuring of
the domain tables.


#  UCL-CS.AC.UK domain table


# Map all known names below the (transparent) UCL-CS.AC.UK
# Thus string 44b maps into DOMAIN 44b.UCL-CS.AC.UK
# which maps into HOST 44b
# which is found in the ring channel  table
# (or in the case of 44a in the local channel  table or 44a-list
# in the list channel table)
44a:44a
44b:44b
44c:44c
.....
44a-list:44a-list
44b-list:44b-list
44c-list:44c-list
....





#  ARPA Domain table


# idis is at top of file as it is used as a relay host
#  A relays entry must come before any hosts realying
# through it to get the official name correct
usc-isid:usc-isid.arpa
#
#  The following hosts  won't talk TCP to UCL
# (or used not to at least).  Therefore we palm them off
# on someone else (boo hiss)
ada-vax:ada-vax.arpa,usc-isid.arpa
isi-vaxb:ada-vax.arpa,usc-isid.arpa
ajpo:ada-vax.arpa,usc-isid.arpa
vaxb:ada-vax.arpa,usc-isid.arpa
almsa-1:almsa-1.arpa,usc-isid.arpa
alta-coma:alta-coma.arpa,usc-isid.arpa
ames-tss:ames-tss.arpa,usc-isid.arpa
ames-67:ames-tss.arpa,usc-isid.arpa
#
# Here are all of the names from hosts.txt
# Official name first.  Host name on RHS
alta-coma:alta-coma.arpa
yuma:yuma.arpa
bbncca:bbncca.arpa
bbnccb:bbnccb.arpa
bbnccc:bbnccc.arpa
bbnccd:bbnccd.arpa
bbn-labs-admin:bbn-labs-admin.arpa
bbn-labs:bbn-labs-admin.arpa
bbnl:bbn-labs-admin.arpa
bbncck:bbncck.arpa


# Mailnet Domain table


# All domains are reached through MIT relay
# the official name of mit-multics si found in
# the Arpa domain
# There is no need for a mailnet channel
carnegie:carnegie.mailnet,mit-multics.arpa
cornella:cornella.mailnet,mit-multics.arpa
dickinson:dickinson.mailnet,mit-multics.arpa
durham:durham.mailnet,mit-multics.arpa
educom:educom.mailnet,mit-multics.arpa



# AC.UK Domain table


#  Lots of amazing names for UCL at the top
ucl-cs:ucl-cs
ucl:ucl-cs
zuxa:ucl-cs
zuxa80:ucl-cs
research:ucl-cs
nss:ucl-cs
unknown-host:ucl-cs
zuxc:ucl-cs
ucl-niftp:ucl-cs
ucl-tg:ucl-cs
src-44a:44c
#
#    Some odinary domain names
abxa:abxa
apca:apca
baca:baca
bdga:bdga
bdgb:bdgb
bedford:bedford
bgxa:bgxa
#  These entries will map HCIG.NOTT, HCIG.NOTTINGHAM
#  NOTT, NOTTINGHAM, HCIG (and all the permutations with .AC or
#  .AC.UK appended) into HCIG.NOTT.AC.UK
hcig.nott:hcig.nott
hcig.nottingham:hcig.nott
nottingham:hcig.nott
nott:hcig.nott
hcig:hcig.nott
#
#  Some examples of Relayed connections.
# Note that the rightmost component should be the host
#  directly connected to.  The leftmost component should be
# The Name required for 'Host' format output
dlia:dlia,rlgb
rlia:rlia,rlgb
rlib:rlib,rlgb
camphx:camphx,caga
caxa:camphx,caga
#  Camring is routed first ot caga, then camjenny, finally to camring
camring:camring,camjenny.ac.uk,caga
camjenny:camjenny,caga
camsteve:camsteve,caga
ecsvax:ecsvax,edxa
rco:rco,edxa
edxd:rco,edxa
eagle:eagle,ukc.ac.uk,rco.ac.uk,edxa
ktda:eagle,ukc.ac.uk,rco.ac.uk,edxa
kent:eagle,ukc.ac.uk,rco.ac.uk,edxa
#
#    Hosts connected to over the PSS Network
aucc:aucc
cardiff:cardiff
cardf:cardiff
essex:essex


#  Top level domain

#  UUCP and Bitnet through berkeley gateway
#  Note that the HOST name of the relay must be given
uucp:ucb-vax.arpa
bitnet:ucb-vax.arpa
# Mailnet relay through MIT.  Domains in the mailnet table can
# be abbreviated.  Others will only work if specified in full
mailnet:mit-multics.arpa

