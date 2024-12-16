#### Доступ к серверу:
host: psw-ci.ft.local  
user: user  
pass: PSWpass01  
root pass: PSWpass01  

### 1. Подключение к серверу  
`ssh user@psw-ci.ft.local`

### 2. Переключение на пользователя gitlab-runner:
`su` + password  
`su gitlab-runner` 

#### Создание контейнера из образа и запуск контейнера
`docker run -it name:tag` 
`docker run --privileged --rm -it tfortis_os:1.0`

`docker images`  
`docker commit -m "start build clear image" 91ecded6e28f tfortis_build:0.1`  
`docker build -t tfortis_OS -f Dockerfile .` 
`docker stop $(docker ps -a -q)`   

`docker rmi -f psw-tets`  
`docker system prune`

apt install liblzma-dev

## Artifacts
mkdir fw_tfortis
mount -t cifs -o rw,file_mode=0777,dir_mode=0777,username=tfortis_ci,password=EaLaT7Hz //10.160.20.190/fw_tfortis ./fw_tfortis



