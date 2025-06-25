extends Node3D

var xr_interface: XRInterface
var last_pitch = 0
var last_roll = 0
var lerp_speed := 5.0  # Adjust for smoother interpolation

func _ready():
	xr_interface = XRServer.find_interface("OpenXR")
	if xr_interface and xr_interface.is_initialized():
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
		get_viewport().use_xr = true

func _process(delta):
	var roll = Input.get_joy_axis(0, 0)
	var pitch = Input.get_joy_axis(0, 1)
	
	roll *= 90
	pitch *= 90
	
	roll = 0.9 * last_roll + 0.1 * roll
	pitch = 0.9 * last_pitch + 0.1 * pitch
	last_roll = roll
	last_pitch = pitch
	
	print("Roll: ", roll, "\tPitch: ", pitch)
	
	$Floor.rotation_degrees.x = -pitch
	$Floor.rotation_degrees.z = -roll

func _starting_the_game():
	get_tree().change_scene("res://main1.tscn")
	if ball_falls_down():
		_starting_the_game()
	elif contact_coin():
		_level_2()

func _level_2():
	get_tree().change_scene("res://main2.tscn")
	if ball_falls_down():
		_starting_the_game()
	elif contact_coin():
		_level_3()

func _level_3():
	get_tree().change_scene("res://main3.tscn")
	if ball_falls_down() or contact_coin():
		_starting_the_game()

func ball_falls_down() -> bool:
	return $Ball.global_transform.origin.y < -10

func contact_coin() -> bool:
	return $Ball.is_colliding_with($Coin)  # Replace with actual collision logic
