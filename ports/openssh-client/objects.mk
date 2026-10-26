SSHOBJS := ssh.o readconf.o clientloop.o sshtty.o  	sshconnect.o sshconnect2.o mux.o ssh-pkcs11.o ssh-sk-client.o
SFTP_OBJS := sftp.o sftp-usergroup.o progressmeter.o sftp-common.o sftp-client.o sftp-glob.o ssherr-nolibcrypto.o
SCP_OBJS := scp.o progressmeter.o sftp-common.o sftp-client.o sftp-glob.o ssherr-nolibcrypto.o
