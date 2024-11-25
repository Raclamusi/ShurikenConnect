# pragma once

# include <Siv3D.hpp>

/// @brief スマートフォンの画面への1つの接触点の情報
struct SPTouch
{
	/// @brief 画面に触れた領域にもっとも接近して囲むような楕円
	Ellipse region;

	/// @brief 楕円の回転角度を [0, π/2) の弧度法で表した値
	double angle;

	/// @brief 接触面に与えられた圧力の大きさを 0.0 から 1.0 の間で表した値
	float force;

	/// @brief 接触点を識別する固有の値
	int32 id;
};
