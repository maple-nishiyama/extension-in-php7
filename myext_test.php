<?php
$a = 10;
$b = 5;

$c = my_sum($a, $b);
echo "$c\n";

$ext = new Myext([1, 2, 3, 4]);

var_dump($ext);

$c = $ext->my_sum_method(3, 4);
echo "$c\n";

var_dump($ext->readBuffer());

$c = Myext::zeros(10);
var_dump($c->readBuffer());
