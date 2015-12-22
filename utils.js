function toDegrees (angle) {
	return angle * (180 / Math.PI);
}
function toRadians (angle) {
	return angle * Math.PI / 180;
}
function sign(v) {
	if (v > 0)
		return 1;
	else if (v < 0)
		return -1;
	else
		return 0;
}
