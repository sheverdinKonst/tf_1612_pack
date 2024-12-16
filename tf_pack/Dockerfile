FROM debian:11
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt install make
RUN apt install openssh-server -y
RUN apt install checkinstall
RUN apt install autoconf
RUN apt install autoconf
RUN apt install automake
RUN apt-get install build-essential -y libncurses5-dev zlib1g-dev
RUN apt-get install lib32z1 -y
RUN apt-get install -y gawk gcc-multilib flex git gettext libssl-dev
RUN apt-get install -y python3-distutils cmake
RUN apt-get install -y rsync unzip file
RUN apt-get install wget
RUN apt -y install device-tree-compiler
RUN apt -y install nano
RUN apt -y install vim


RUN mkdir /home/t_fortis && cd /home/t_fortis
RUN git clone http://sheverdin:7319837kon@gitlab.forttel.ru/TFortis/FirmWare/tfortis-realtek/open_wrt_test3.git

#FORCE_UNSAFE_CONFIGURE=1
