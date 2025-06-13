<?php
session_start();

if ($_SESSION['auth']==false)
{
	echo'
	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd">
	<html>
		<head>
			<meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no">
			<script src="/js/jquery.min.js"></script>
			<link rel="stylesheet" href="/css/style.css" type="text/css">
			<link rel="icon" href="/img/icon.png" type="image/x-icon">
			<title>Вход</title>
		</head>
		<body>
			<div class="p_1">
				<div class="m_1 w_100">
					<div class="t_1 ta_c">Вход</div>
					<div class="m_4"><b>Логин</b></div>
					<div class="m_5">
						<input type="text" class="input w_100 login" placeholder="Введите логин">
					</div>
					<div class="m_4"><b>Пароль</b></div>
					<div class="m_5">
						<input type="password" class="input w_100 password" placeholder="Введите пароль">
						<div class="m_4 btn_2 w_100 auth">Войти</div>
					</div>
					<div class="m_4 ta_c info"></div>
				</div>
			</div>
		</body>
	</html>';
}

if ($_SESSION['auth']==true)
{
	$rotate=file_get_contents(dirname(__FILE__).'/rotate.txt');
	$compass=file_get_contents(dirname(__FILE__).'/compass.txt');
	$login=file_get_contents(dirname(__FILE__).'/login.txt');
	$current_video=file_get_contents(dirname(__FILE__).'/current_video.txt');
    $screensaver=file_get_contents(dirname(__FILE__).'/screensaver.txt');
	$smoothing=file_get_contents(dirname(__FILE__).'/smoothing.txt');

	echo'
	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd">
	<html>
		<head>
			<meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no">
			<script src="/js/jquery.min.js"></script>
			<script src="/js/jquery.form.min.js"></script>
			<link rel="stylesheet" href="/css/style.css" type="text/css">
			<link rel="icon" href="/img/icon.png" type="image/x-icon">
			<title>Настройки</title>
		</head>
		<body>
			<div class="p_1 m_0a">
				<div class="m_1 w_100">
					<div class="b_7">
						<div class="p_6">
							После нажатия на Калибровку у вас есть 10 секунд чтобы покрутить прибор влево и вправо до упора, после этого можно соседними стрелками установить первоначальное положение видео панорамы.
						</div>
					</div>
					<div class="t_1 ta_c m_4">Управление</div>
					<table class="w_100">
						<tr>
							<td>
								<center>
									<div class="m_4 btn_2 w_3 signal" param="turn_left">&#9668;</div>
									<div class="m_4 btn_2 w_3 signal" param="zoom_minus">&#8211;</div>
									<div class="m_4 btn_2 w_3 signal" param="shift_lower">&#9660;</div>
								</center>
							</td>
							<td>
                            	<center>
	                            	<div class="m_4 btn_2 w_4 signal" param="calibration">Калибровка</div>
	                            	<div class="m_4 btn_none">Приближение</div>
	                            	<div class="m_4 btn_none">Стерео сдвиг</div>
                            	</center>
							</td>
							<td>
                            	<center>
	                            	<div class="m_4 btn_2 w_3 signal" param="turn_right">&#9658;</div>
	                            	<div class="m_4 btn_2 w_3 signal" param="zoom_plus">+</div>
	                            	<div class="m_4 btn_2 w_3 signal" param="shift_upper">&#9650;</div>
                            	</center>
							</td>
						</tr>
					</table>
				</div>


				<div class="m_1 w_100">
					<div class="t_1 ta_c">Настройки</div>
					<div class="m_4"><b>Загрузка видео</b></div>
					<form id="formUpload" method="post" enctype="multipart/form-data" action="/data.php">
						<input type="hidden" name="type" value="3">
						<input type="file" name="file[]" class="file" id="file" title="Прикрепить видео">
					</form>
					<div class="m_4 ta_c info_file dn"></div>

					<div class="m_4"><b>Список видео</b></div>
					<select class="select video_list w_100">';
						$hide_file=array('.','..');
						$scan=scandir(dirname(__FILE__).'/player/');
						foreach ($scan as $s)
						{
							if (!in_array($s,$hide_file))
							{
								if ($s==$current_video) {$this_video_1='&#9658; '; $this_video_2=' — Выбрано для воспроизведения'; $this_video_selected='selected';} else {$this_video_1=''; $this_video_2=''; $this_video_selected='';}
								echo'
								<option '.$this_video_selected.' value="'.$s.'">'.$this_video_1.$s.$this_video_2.'</option>';
							}
						}
					echo'
					</select>
					<div class="m_4 btn_2 w_100 video_play dn">Воспроизводить</div>
					<div class="m_4 btn_2 w_100 video_del dn" style="background:red;">Удалить</div>
					<div class="m_4 ta_c info_video"></div>

					<div class="m_4"><b>Повернуть экран</b></div>
					<div class="m_5">';
						$rotate_val=array('-1','0','1');
						$rotate_text=array('Влево','Норма','Вправо');
						echo'
						<select class="select rotate w_100">';
							foreach ($rotate_val as $key_rv=>$rv)
							{
								if ($rotate_val[$key_rv]==$rotate) {$select_rv='selected';} else {$select_rv='';}
								echo'
								<option '.$select_rv.' value="'.$rotate_val[$key_rv].'">'.$rotate_text[$key_rv].'</option>';
							}
						echo'
						</select>
					</div>
					<div class="m_4"><b>Задействовать компас</b></div>
					<div class="m_5">
						<div class="m_5">';
						$compass_val=array('0','1');
						$compass_text=array('Выключено','Включено');
						echo'
						<select class="select compass w_100">';
							foreach ($compass_val as $key_cv=>$cv)
							{
								if ($compass_val[$key_cv]==$compass) {$select_cv='selected';} else {$select_cv='';}
								echo'
								<option '.$select_cv.' value="'.$compass_val[$key_cv].'">'.$compass_text[$key_cv].'</option>';
							}
						echo'
						</select>
					</div>
					<div class="m_4"><b>Скринсейвер</b></div>
					<div class="m_5">
						<div class="m_5">
							<input type="number" min="1" class="input w_100 screensaver" placeholder="Скринсейвер в минутах" value="'.$screensaver.'">
						</div>
					</div>
					<div class="m_4"><b>Сглаживание</b></div>
					<div class="m_5">
						<div class="m_5">
							<input type="number" min="0" max="1" class="input w_100 smoothing" placeholder="Фактор сглаживания" value="'.$smoothing.'" step="0.01">
						</div>
					</div>
					<div class="m_4 btn_2 w_100 save">Сохранить</div>
					<div class="m_4 ta_c info"></div>
				</div>

				<div class="m_1 w_100">
					<div class="t_1 ta_c">Доступ</div>
					<div class="m_4"><b>Логин</b></div>
					<div class="m_5">
						<input type="text" class="input w_100 login" placeholder="Новый логин" value="'.$login.'">
						<div class="m_4 btn_2 w_100 save_login">Сохранить</div>
						<div class="m_4 ta_c info_login"></div>
					</div>
					<div class="m_4"><b>Пароль</b></div>
					<div class="m_5">
						<input type="password" class="input w_100 password_1" placeholder="Новый пароль">
						<input type="password" class="input w_100 m_5 password_2" placeholder="Повторите пароль">
						<div class="m_4 btn_2 w_100 save_password">Сохранить</div>
						<div class="m_4 ta_c info_password"></div>
					</div>
				</div>

				<div class="m_1 btn_2 w_100 exit" style="background:gray;">Выход</div>

			</div>
		</body>
	</html>';
}
?>

<script>
$(document).on("click", ".auth", function(event){
	event.preventDefault()
	var type="1";
	var login=$(".login").val();
	var password=$(".password").val();
	$.post("/data.php", {"type":type,"login":login,"password":password}, function(show){
		if (show.errors)
		{
			$(".info").show();
	      	$(".info").html(show.errors).delay(3000).fadeOut();
		}
		if (show.success)
		{
			$(".auth").hide();
			location.reload();
		}
	},"json");
});

$(document).on("click", ".save", function(event){
	var type="2";
	var rotate=$(".rotate").val();
	var compass=$(".compass").val();
	var wi_fi_ssid=$(".wi_fi_ssid").val();
	var wi_fi_password=$(".wi_fi_password").val();
	var screensaver=$(".screensaver").val();
	var smoothing=$(".smoothing").val();
	$.post("/data.php", {"type":type,"rotate":rotate,"compass":compass,"wi_fi_ssid":wi_fi_ssid,"wi_fi_password":wi_fi_password,"screensaver":screensaver,"smoothing":smoothing}, function(show){
		if (show.errors)
		{
			$(".info").show();
	      	$(".info").html(show.errors).delay(3000).fadeOut();
		}
	},"json");
});

$(document).on("change", "#file", function(event){
	$(".info_file").show();
	$(".info_file").html('<img src="/img/load.gif">');
	$("#formUpload").ajaxForm(function(data){
        var type="8";
        $.post("/data.php", {"type":type}, function(show){
			if (show.info)
			{
				$(".video_list").html(show.list);
				$(".info_file").show();
		      	$(".info_file").html(show.info).delay(3000).fadeOut();
			}
		},"json");
        //$(".info_file").html(data);
 	}).submit();
});

$(document).on("click", ".save_login", function(event){
	var type="4";
	var login=$(".login").val();
	$.post("/data.php", {"type":type,"login":login}, function(show){
		if (show.info)
		{
			$(".info_login").show();
	      	$(".info_login").html(show.info).delay(3000).fadeOut();
		}
	},"json");
});

$(document).on("click", ".save_password", function(event){
	var type="5";
	var password_1=$(".password_1").val();
	var password_2=$(".password_2").val();
	$.post("/data.php", {"type":type,"password_1":password_1,"password_2":password_2}, function(show){
		if (show.info)
		{
			$(".info_password").show();
	      	$(".info_password").html(show.info).delay(3000).fadeOut();
		}
	},"json");
});

$(document).on("change", ".video_list", function(event){
	$(".video_play, .video_del").show();
});

$(document).on("click", ".video_play", function(event){
	var type="6";
	var video=$(".video_list").val();
	$.post("/data.php", {"type":type,"video":video}, function(show){
		if (show.info)
		{
	      	$(".video_list").html(show.list);
	      	$(".info_video").show();
	      	$(".info_video").html(show.info).delay(3000).fadeOut();
	      	$(".video_play, .video_del").hide();
		}
	},"json");
});

$(document).on("click", ".video_del", function(event){
	if(confirm('Действительно удалить видео?'))
   	{
    	var type="7";
    	var video=$(".video_list").val();
    	$.post("/data.php", {"type":type,"video":video}, function(show){
			if (show.info)
			{
		      	$(".video_list").html(show.list);
		      	$(".info_video").show();
		      	$(".info_video").html(show.info).delay(3000).fadeOut();
		      	$(".video_play, .video_del").hide();
			}
		},"json");
	}
});

$(document).on("click", ".signal", function(event){
	var type="9";
	var signal=$(this).attr("param");
	$.post("/data.php", {"type":type,"signal":signal}, function(show){

	});
});

$(document).on("click", ".exit", function(event){
	var type="10";
	$.post("/data.php", {"type":type}, function(show){
		if (show)
		{
			location.reload();
		}
	});
});

</script>