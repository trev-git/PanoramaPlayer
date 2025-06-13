<?php
// IP и порт для прослушивания
$ip = '127.0.0.1';
$port = 12345;

// Создание UDP-сокета
$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
if (!$sock) {
    die("Не удалось создать сокет: " . socket_strerror(socket_last_error()) . "\n");
}

// Привязка сокета к IP и порту
if (!socket_bind($sock, $ip, $port)) {
    die("Не удалось привязать сокет: " . socket_strerror(socket_last_error($sock)) . "\n");
}

echo "UDP-сервер запущен на $ip:$port\n";

while (true) {
    // Получение данных
    $buf = '';
    $from = '';
    $port = 0;

    // Ждём входящее сообщение (максимум 512 байт)
    $bytes = socket_recvfrom($sock, $buf, 512, 0, $from, $port);
    if ($bytes === false) {
        echo "Ошибка при получении данных: " . socket_strerror(socket_last_error($sock)) . "\n";
        continue;
    }

    echo "Получено сообщение от $from:$port — $buf\n";
}

// Закрытие сокета (никогда не дойдём сюда, но на всякий случай)
socket_close($sock);
?>