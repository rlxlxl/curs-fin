# Развертывание проекта на сервере через Docker

## Быстрый старт

### 1. Подключитесь к серверу по SSH

```bash
ssh username@your-server-ip
```

### 2. Установите Docker и Docker Compose на сервере

#### Ubuntu/Debian:
```bash
# Обновление пакетов
sudo apt-get update

# Установка зависимостей
sudo apt-get install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release

# Добавление официального GPG ключа Docker
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

# Настройка репозитория
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# Установка Docker
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# Добавление пользователя в группу docker (чтобы не использовать sudo)
sudo usermod -aG docker $USER
newgrp docker

# Проверка установки
docker --version
docker compose version
```

#### CentOS/RHEL:
```bash
# Установка зависимостей
sudo yum install -y yum-utils

# Добавление репозитория Docker
sudo yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo

# Установка Docker
sudo yum install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# Запуск Docker
sudo systemctl start docker
sudo systemctl enable docker

# Добавление пользователя в группу docker
sudo usermod -aG docker $USER
newgrp docker
```

### 3. Копирование проекта на сервер

#### Вариант 1: Через SCP (с локальной машины)
```bash
# Создайте архив проекта (на локальной машине)
cd "/Users/slavavikentev/Desktop/учеба/семестр 3/прога"
tar -czf curstest2.tar.gz curstest2 \
  --exclude='build' \
  --exclude='*.o' \
  --exclude='.git' \
  --exclude='*.docx'

# Скопируйте на сервер
scp curstest2.tar.gz username@your-server-ip:~/

# На сервере распакуйте
ssh username@your-server-ip
tar -xzf curstest2.tar.gz
cd curstest2
```

#### Вариант 2: Через rsync (рекомендуется)
```bash
# С локальной машины
rsync -avz --exclude='build' --exclude='*.o' --exclude='.git' --exclude='*.docx' \
  "/Users/slavavikentev/Desktop/учеба/семестр 3/прога/curstest2/" \
  username@your-server-ip:~/curstest2/
```

#### Вариант 3: Через Git (если проект в репозитории)
```bash
# На сервере
git clone your-repository-url
cd curstest2
```

### 4. Запуск через Docker Compose

```bash
# Перейдите в директорию docker
cd ~/curstest2/docker

# Запустите контейнеры
docker compose up -d

# Проверьте статус
docker compose ps

# Просмотрите логи
docker compose logs -f
```

### 5. Проверка работы

```bash
# Проверьте, что контейнеры запущены
docker compose ps

# Проверьте логи
docker compose logs web
docker compose logs postgres

# Проверьте доступность
curl http://localhost:8080
```

Откройте в браузере: `http://your-server-ip:8080`

## Настройка файрвола

```bash
# Ubuntu/Debian (ufw)
sudo ufw allow 8080/tcp
sudo ufw reload

# CentOS/RHEL (firewalld)
sudo firewall-cmd --permanent --add-port=8080/tcp
sudo firewall-cmd --reload

# Или iptables
sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT
```

## Обновление проекта

При обновлении кода:

```bash
# 1. Остановите контейнеры
cd ~/curstest2/docker
docker compose down

# 2. Обновите код (через git, rsync или scp)

# 3. Пересоберите и запустите
docker compose build --no-cache
docker compose up -d

# 4. Проверьте логи
docker compose logs -f web
```

## Полезные команды

```bash
# Просмотр логов
docker compose logs -f          # Все сервисы
docker compose logs -f web      # Только веб-сервер
docker compose logs -f postgres # Только PostgreSQL

# Остановка контейнеров
docker compose stop

# Запуск контейнеров
docker compose start

# Перезапуск
docker compose restart

# Остановка и удаление контейнеров (данные БД сохранятся в volume)
docker compose down

# Остановка и удаление контейнеров + volumes (удалит все данные!)
docker compose down -v

# Просмотр использования ресурсов
docker stats

# Вход в контейнер PostgreSQL
docker compose exec postgres psql -U postgres -d infosec_db

# Проверка данных в БД
docker compose exec postgres psql -U postgres -d infosec_db -c "SELECT COUNT(*) FROM integrators;"
```

## Настройка автозапуска при перезагрузке сервера

Docker Compose автоматически перезапускает контейнеры при перезагрузке, если они запущены через `docker compose up -d`.

Для более надежного решения используйте systemd:

Создайте файл `/etc/systemd/system/infosec-docker.service`:

```bash
sudo nano /etc/systemd/system/infosec-docker.service
```

Содержимое:
```ini
[Unit]
Description=InfoSec Docker Compose
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/home/your-username/curstest2/docker
ExecStart=/usr/bin/docker compose up -d
ExecStop=/usr/bin/docker compose down
User=your-username
Group=docker

[Install]
WantedBy=multi-user.target
```

Затем:
```bash
sudo systemctl daemon-reload
sudo systemctl enable infosec-docker
sudo systemctl start infosec-docker
```

## Резервное копирование базы данных

```bash
# Создание бэкапа
docker compose exec postgres pg_dump -U postgres infosec_db > backup_$(date +%Y%m%d_%H%M%S).sql

# Восстановление из бэкапа
docker compose exec -T postgres psql -U postgres infosec_db < backup_file.sql
```

## Изменение порта (если 8080 занят)

Отредактируйте `docker/docker-compose.yml`:

```yaml
web:
  ...
  ports:
    - "8081:8080"  # Внешний порт:внутренний порт
```

Затем перезапустите:
```bash
docker compose down
docker compose up -d
```

## Решение проблем

### Контейнеры не запускаются
```bash
# Проверьте логи
docker compose logs

# Проверьте, не занят ли порт
sudo netstat -tlnp | grep 8080
# или
sudo ss -tlnp | grep 8080
```

### База данных не инициализируется
```bash
# Удалите volume и пересоздайте
docker compose down -v
docker compose up -d

# Проверьте логи PostgreSQL
docker compose logs postgres
```

### Проблемы с правами доступа
```bash
# Убедитесь, что пользователь в группе docker
sudo usermod -aG docker $USER
newgrp docker
```

## Автоматический скрипт развертывания

Создайте файл `deploy-docker.sh` на сервере:

```bash
#!/bin/bash
cd ~/curstest2/docker
docker compose down
docker compose build --no-cache
docker compose up -d
docker compose logs -f
```

Сделайте исполняемым:
```bash
chmod +x deploy-docker.sh
```

Использование:
```bash
./deploy-docker.sh
```

