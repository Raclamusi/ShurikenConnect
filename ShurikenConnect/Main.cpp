# include <Siv3D.hpp> // Siv3D v0.6.15
# include "SPInput.hpp"

enum class Mode
{
	QRCode,
	Test,
	Game,
};

struct Shuriken
{
	Vec2 pos;
	Vec2 startPos;
	Vec2 targetPos;
	double time;
	double angle;
	size_t textureIndex;
};

struct TouchVelocity
{
	Vec2 pos;
	Vec2 velocity;
	double time;
};

void Main()
{
	Window::Resize(1000, 600);

	constexpr uint16 DefaultPort = 50000;

	SPInput input{ DefaultPort };

	TextEditState portEditState{ Format(DefaultPort) };

	Mode mode = Mode::QRCode;

	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };
	font.setBufferThickness(3);

	URL url = input.getURL();
	DynamicTexture qrTexture{ QR::MakeImage(QR::EncodeText(url)) };

	const Texture enemyTexture{ Resource(U"image/alien_ufo.png") };
	const Texture smartphoneTexture{ Resource(U"image/computer_smartphone1_icon.png") };
	const Array shurikenTextures
	{
		Texture{ Resource(U"image/ninja_syuriken1.png") },
		Texture{ Resource(U"image/ninja_syuriken2.png") },
		Texture{ Resource(U"image/ninja_syuriken3.png") },
		Texture{ Resource(U"image/ninja_syuriken4.png") },
		Texture{ Resource(U"image/ninja_syuriken5.png") },
	};
	DiscreteDistribution shurikenDistribution{ 87, 10, 1, 1, 1 };

	bool gameStarted = false;
	bool gameOver = false;
	double time = 0;
	uint32 score = 0;
	Array<Vec2> enemies;
	Array<Shuriken> shurikens;
	HashTable<int32, TouchVelocity> touchVelocities;

	while (System::Update())
	{
		input.update();

		Scene::Rect().right().movedBy(-200, 0).draw(Palette::White);
		if (SimpleGUI::Button(U"Show QR Code", Vec2{ (Scene::Width() - 180), 40}, 160))
		{
			mode = Mode::QRCode;
		}
		if (SimpleGUI::Button(U"Show Touches", Vec2{ (Scene::Width() - 180), 100 }, 160))
		{
			mode = Mode::Test;
		}
		if (SimpleGUI::Button(U"Play Game", Vec2{ (Scene::Width() - 180), 160 }, 160))
		{
			mode = Mode::Game;
		}

		SimpleGUI::Headline(U"Port", Vec2{ (Scene::Width() - 180), 280 }, 160);
		SimpleGUI::TextBox(portEditState, Vec2{ (Scene::Width() - 180), 320 }, 160);
		if (SimpleGUI::Button(U"Reset Port", Vec2{ (Scene::Width() - 180), 380 }, 160))
		{
			input.reset(ParseOr<uint16>(portEditState.text, DefaultPort));
			url = input.getURL();
			qrTexture.fill(QR::MakeImage(QR::EncodeText(url)));
		}

		if (mode == Mode::QRCode)
		{
			font(U"Scan to Connect!").drawAt(48, 400, 60, Palette::White);
			qrTexture.resized(400).draw(Arg::center(400, 300));
			font(url).drawAt(24, 400, 540, Palette::White);
		}
		else if (mode == Mode::Test)
		{
			const auto size = input.screenSize();
			if (size.x != 0 && size.y != 0)
			{
				const auto scale = Min((700.0 / size.x), (500.0 / size.y));
				const Vec2 offset{ (400 - size.x * scale * 0.5), (300 - size.y * scale * 0.5) };

				const Transformer2D transformer{ Mat3x2::Scale(scale).translated(offset) };
				
				const RectF screenRect{ size };
				const auto stretch = ((size.x < size.y) ? Vec2{ 5, 30 } : Vec2{ 30, 5 });
				screenRect.stretched(stretch / scale).rounded(20 / scale).draw(Palette::White);
				screenRect.draw(Palette::Black);

				for (const auto& touch : input.touches())
				{
					const Transformer2D touchTransformer{ Mat3x2::Rotate(touch.angle, touch.region.center) };
					touch.region.draw(HSV{ (240 * (1 - touch.force)) });
				}
				for (const auto& touch : input.touches())
				{
					font(touch.id).drawAt(TextStyle::Outline(0.2, Palette::Black), (20 / scale), touch.region.bottom().movedBy(0, 20), Palette::White);
				}
			}
		}
		else if (mode == Mode::Game)
		{
			const ScopedViewport2D viewport{ 800, 600 };

			if (gameOver && MouseL.down())
			{
				enemies.clear();
				shurikens.clear();
				touchVelocities.clear();
				score = 0;
				gameOver = false;
			}

			time += Scene::DeltaTime();

			// 敵や手裏剣の移動
			for (auto& pos : enemies)
			{
				pos.y += ((50 + 2 * time) * Scene::DeltaTime());
			}
			enemies.remove_if([](const Vec2& pos) { return (pos.y > 700); });
			for (auto& shuriken : shurikens)
			{
				shuriken.time += Scene::DeltaTime();
				shuriken.pos = EaseOut(Easing::Quad, shuriken.startPos, shuriken.targetPos, shuriken.time);
				shuriken.angle += (720_deg * Scene::DeltaTime());
			}
			shurikens.remove_if([](const Shuriken& shuriken) { return (shuriken.time > 1); });

			// 手裏剣の攻撃
			enemies.remove_if([&](const Vec2& pos)
				{
					for (const auto& shuriken : shurikens)
					{
						if (Circle{ shuriken.pos, 50 }.intersects(Circle{ pos, 50 }))
						{
							if (gameStarted)
							{
								score += 1;
							}
							return true;
						}
					}
					return false;
				});

			// ゲームオーバー判定
			if (enemies.any([](const Vec2& pos) { return (pos.y > 600); }))
			{
				gameStarted = false;
				gameOver = true;
				shurikens.clear();
			}

			if (not gameOver)
			{
				// タッチの処理
				for (const auto& touch : input.touches())
				{
					if (const auto it = touchVelocities.find(touch.id); it != touchVelocities.end())
					{
						if (it->second.pos != touch.region.center)
						{
							it->second.velocity = (touch.region.center - it->second.pos) / (time - it->second.time);
							it->second.pos = touch.region.center;
							it->second.time = time;
						}
					}
					else
					{
						touchVelocities[touch.id] = TouchVelocity{ touch.region.center, Vec2{ 0, 0 }, time };
					}
				}

				// 手裏剣の追加
				for (const auto id : input.endedTouches())
				{
					if (const auto it = touchVelocities.find(id); it != touchVelocities.end())
					{
						const Vec2 pos{ 400, 600 };
						const Vec2 targetPos = (pos + it->second.velocity * 0.2);
						const auto textureIndex = shurikenDistribution(GetDefaultRNG());
						if (not gameStarted)
						{
							gameStarted = true;
							time = 0;
						}
						shurikens.push_back(Shuriken{ pos, pos, targetPos, 0, 0, textureIndex });
						touchVelocities.erase(it);
					}
				}
				for (const auto id : input.canceledTouches())
				{
					touchVelocities.erase(id);
				}

				// 敵の追加
				if (gameStarted)
				{
					if (RandomBool(0.1 + 0.005 * time))
					{
						enemies.push_back(Vec2{ Random(50, 750), -50 });
					}
				}
			}

			Rect{ 800, 600 }.draw(Palette::Midnightblue);

			font(score).drawAt(128, 400, 300, ColorF{ Palette::White, 0.5 });

			smartphoneTexture.resized(200).drawAt(400, 600);

			for (const auto& pos : enemies)
			{
				enemyTexture.resized(100).drawAt(pos);
			}
			for (const auto& shuriken : shurikens)
			{
				Circle{ shuriken.pos, 50 }.draw(Palette::White, ColorF{ Palette::White, 0 });
				shurikenTextures[shuriken.textureIndex].resized(100).rotated(shuriken.angle).drawAt(shuriken.pos);
			}

			if (not gameStarted)
			{
				Rect{ 800, 600 }.draw(ColorF{ Palette::Black, 0.5 });

				if (gameOver)
				{
					font(U"Game Over!").drawAt(64, 400, 240, Palette::White);
					font(U"Score: {}"_fmt(score)).drawAt(48, 400, 300, Palette::White);
					font(U"Click Screen to Reset").drawAt(32, 400, 350, Palette::White);
				}
				else
				{
					font(U"Flick to throw shuriken!").drawAt(48, 400, 300, Palette::White);
				}
			}
		}
	}
}
