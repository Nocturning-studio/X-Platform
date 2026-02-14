#include <xrMath/xrMath.h>

#include <iostream>
#include <cassert>
#include <cmath>
#include <chrono>
#include <thread>

int main()
{
	std::cout << "Running xrMath Integration Tests..." << std::endl;

	// 1. Создаем вращение на 90 градусов по Y через Кватернион
	xray::quaternion q_orig;
	xray::float3 axis;
	axis.set(0, 1, 0);
	q_orig.rotation(axis, xray::PI_DIV_2); // 90 deg

	// 2. Конвертируем в Матрицу
	xray::float4x4 m;
	m.set(q_orig);

	// Проверяем матрицу (cos(90)=0, sin(90)=1)
	// Матрица поворота Y:
	// [ cos  0  sin ] -> [ 0  0  1 ]
	// [  0   1   0  ] -> [ 0  1  0 ]
	// [ -sin 0  cos ] -> [-1  0  0 ]
	assert(std::abs(m._11) < xray::EPS);		 // cos(90) ~ 0
	assert(std::abs(m._13 - 1.0f) < xray::EPS);	 // sin(90) ~ 1
	assert(std::abs(m._31 - (-1.0f)) < xray::EPS); // -sin(90) ~ -1

	// 3. Конвертируем обратно в Кватернион
	xray::quaternion q_new;
	q_new.set(m);

	// 4. Сравниваем исходный и новый кватернионы
	// (учитываем, что q и -q представляют одно и то же вращение, но здесь знаки должны совпасть)
	assert(std::abs(q_orig.w - q_new.w) < xray::EPS);
	assert(std::abs(q_orig.y - q_new.y) < xray::EPS);

	std::cout << "[OK] Matrix <-> Quaternion conversion passed!" << std::endl;

	xray::plane plane;
	// Плоскость, смотрящая вверх (Y+), проходящая через Y=5
	xray::float3 p_point;
	p_point.set(0, 5, 0);
	xray::float3 p_norm;
	p_norm.set(0, 1, 0);
	plane.build(p_point, p_norm);

	xray::float3 test_p1;
	test_p1.set(0, 10, 0); // Выше плоскости
	xray::float3 test_p2;
	test_p2.set(0, 0, 0); // Ниже плоскости

	assert(plane.classify(test_p1) > 0);
	assert(plane.classify(test_p2) < 0);
	std::cout << "[OK] Plane tests passed" << std::endl;

	// --- AABB TESTS ---
	xray::box box;
	box.invalidate(); // Сброс

	xray::float3 p1;
	p1.set(-1, -1, -1);
	xray::float3 p2;
	p2.set(1, 1, 1);

	box.extend(p1);
	box.extend(p2);

	// Центр должен быть (0,0,0)
	xray::float3 center;
	box.get_center(center);
	assert(std::abs(center.x) < xray::EPS && std::abs(center.y) < xray::EPS);

	// Проверка попадания
	xray::float3 inside;
	inside.set(0, 0, 0);
	xray::float3 outside;
	outside.set(2, 2, 2);

	assert(box.contains(inside) == true);
	assert(box.contains(outside) == false);
	std::cout << "[OK] AABB tests passed" << std::endl;

	while (true)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	return 0;
}
