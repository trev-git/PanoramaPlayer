<?php
ini_set('max_execution_time',0);
session_start();

if ($_POST['type']==1) //Авторизация
{
	//$login='admin';
	//$password='g9vw2qf';
	$login=file_get_contents(dirname(__FILE__).'/login.txt');
	$password=file_get_contents(dirname(__FILE__).'/password.txt');

	if ((trim(strtolower($_POST['login']))==trim(strtolower($login))) and (md5('salt_'.trim($_POST['password']))==$password))
	{
		$res['success']=1;
		$_SESSION['auth']=true;
	}
	else
	{
		$res['errors']='<span style="color:red;">Неверный логин или пароль</span>';
	}

	echo json_encode($res);
}

if ($_POST['type']==2) //Сохранение параметров
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	file_put_contents(dirname(__FILE__).'/rotate.txt',trim($_POST['rotate']));
	file_put_contents(dirname(__FILE__).'/compass.txt',trim($_POST['compass']));

	$screensaver=preg_replace('/[^0-9]/', '', trim(strip_tags($_POST['screensaver'])));
	if (intval($screensaver)==0) $screensaver=1;
	file_put_contents(dirname(__FILE__).'/screensaver.txt',$screensaver);
	file_put_contents(dirname(__FILE__).'/smoothing.txt',trim($_POST['smoothing']));

	$res['errors']='<span style="color:green;">Сохранено</span>';
	echo json_encode($res);
}

if ($_POST['type']==3) //Загрузка видео
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	/*
	$scan=scandir(dirname(__FILE__).'/file/');
	foreach ($scan as $s)
	{
		unlink(dirname(__FILE__).'/file/'.$s);
	}
	*/

	$file_name=$_FILES['file']['name'][0];
	$file_type=strtolower(pathinfo($_FILES['file']['name'][0], PATHINFO_EXTENSION));
	move_uploaded_file($_FILES['file']['tmp_name'][0], dirname(__FILE__).'/player/'.$file_name);

	file_put_contents(dirname(__FILE__).'/current_video.txt',$file_name);

	echo '<span style="color:green;">Видео-файл загружен</span>';
}

if ($_POST['type']==4) //Сохранение логина
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	if (!trim(strip_tags($_POST['login'])))
	{
		$res['info']='<span style="color:red;">Введите логин</span>';
	}
	else
	{
		file_put_contents(dirname(__FILE__).'/login.txt',trim(strip_tags($_POST['login'])));

		$res['info']='<span style="color:green;">Логин сохранён</span>';
	}
	echo json_encode($res);
}

if ($_POST['type']==5) //Сохранение пароля
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	if ((!trim(strip_tags($_POST['password_1']))) or (!trim(strip_tags($_POST['password_2']))))
	{
		$res['info']='<span style="color:red;">Введите пароль</span>';
	}
	else
	{
		if (trim(strip_tags($_POST['password_1']))!=trim(strip_tags($_POST['password_2'])))
		{
			$res['info']='<span style="color:red;">Пароли не совпадают</span>';
		}
		else
		{
			$password=md5('salt_'.trim(strip_tags($_POST['password_1'])));
			file_put_contents(dirname(__FILE__).'/password.txt',$password);

			$res['info']='<span style="color:green;">Пароль сохранён</span>';
		}
	}
	echo json_encode($res);
}

if ($_POST['type']==6) //Выбор видео для воспроизведения
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	file_put_contents(dirname(__FILE__).'/current_video.txt',$_POST['video']);

	$current_video=file_get_contents(dirname(__FILE__).'/current_video.txt');
	$hide_file=array('.','..');
	$scan=scandir(dirname(__FILE__).'/player/');
	foreach ($scan as $s)
	{
		if (!in_array($s,$hide_file))
		{
			if ($s==$current_video) {$this_video_1='&#9658; '; $this_video_2=' — Выбрано для воспроизведения'; $this_video_selected='selected';} else {$this_video_1=''; $this_video_2=''; $this_video_selected='';}
			$res['list'].='
			<option '.$this_video_selected.' value="'.$s.'">'.$this_video_1.$s.$this_video_2.'</option>';
		}
	}

	$res['info']='<span style="color:green;">Выбрано для воспроизведения</span>';

	echo json_encode($res);
}

if ($_POST['type']==7) //Удаление видео из воспроизведения
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	//Удаление файла
	$scan_del=scandir(dirname(__FILE__).'/player/');
	foreach ($scan_del as $s_d)
	{
    	if ($s_d==$_POST['video'])
    	{
    		unlink(dirname(__FILE__).'/player/'.$s_d);
    	}
	}

	//Если удаляемый файл - это файл который сейчас воспроизводится, то очистить документ
	$current_video_check=file_get_contents(dirname(__FILE__).'/current_video.txt');
	if ($current_video_check==$_POST['video'])
	{
		file_put_contents(dirname(__FILE__).'/current_video.txt','');
	}


	$current_video=file_get_contents(dirname(__FILE__).'/current_video.txt');
	$hide_file=array('.','..');
	$scan=scandir(dirname(__FILE__).'/player/');
	foreach ($scan as $s)
	{
		if (!in_array($s,$hide_file))
		{
			if ($s==$current_video) {$this_video_1='&#9658; '; $this_video_2=' — Выбрано для воспроизведения'; $this_video_selected='selected';} else {$this_video_1=''; $this_video_2=''; $this_video_selected='';}
			$res['list'].='
			<option '.$this_video_selected.' value="'.$s.'">'.$this_video_1.$s.$this_video_2.'</option>';
		}
	}

	$res['info']='<span style="color:green;">Удалено</span>';

	echo json_encode($res);
}

if ($_POST['type']==8) //Вывод списка файлов после загрузки видео
{
	if ($_SESSION['auth']!=true)
	{
		die();
	}

	$current_video=file_get_contents(dirname(__FILE__).'/current_video.txt');
	$hide_file=array('.','..');
	$scan=scandir(dirname(__FILE__).'/player/');
	foreach ($scan as $s)
	{
		if (!in_array($s,$hide_file))
		{
			if ($s==$current_video) {$this_video_1='&#9658; '; $this_video_2=' — Выбрано для воспроизведения'; $this_video_selected='selected';} else {$this_video_1=''; $this_video_2=''; $this_video_selected='';}
			$res['list'].='
			<option '.$this_video_selected.' value="'.$s.'">'.$this_video_1.$s.$this_video_2.'</option>';
		}
	}

	$res['info']='<span style="color:green;">Видео-файл загружен</span>';

	echo json_encode($res);
}

if ($_POST['type']==9) //Сигнал
{
	/*
	if ($_SESSION['auth']!=true)
	{
		die();
	}
	*/

	// Адрес и порт получателя
	$ip = '127.0.0.1';
	$port = '12345';

	// Сообщение для отправки
	$message = trim($_POST['signal']);

	// Создание UDP-сокета
	$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
	/*
	if (!$sock) {
	    die("Не удалось создать сокет: " . socket_strerror(socket_last_error()) . "\n");
	}
	*/

	// Отправка сообщения
	$sent = socket_sendto($sock, $message, mb_strlen($message,'latin1'), 0, $ip, $port);
	/*
	if ($sent === false) {
	    echo "Ошибка отправки: " . socket_strerror(socket_last_error($sock)) . "\n";
	} else {
	    echo "Сообщение отправлено на $ip:$port\n";
	}
	*/

	// Закрытие сокета
	socket_close($sock);

	/*
	$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);

	$msg = $_POST['signal'];
	$len = strlen($msg);

	socket_sendto($sock, $msg, $len, 0, '127.0.0.1', 1234);
	socket_close($sock);

	echo $_POST['signal'];
	*/
}

if ($_POST['type']==10) //Выход
{
	session_start();
	session_unset();
	session_destroy();
	echo 'Выход';
}

?>