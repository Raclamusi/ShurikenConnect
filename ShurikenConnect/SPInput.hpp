# pragma once

# include <Siv3D.hpp>
# include "SPTouch.hpp"

/// @brief スマートフォンからの入力情報を受け取るクラス
class SPInput : Uncopyable
{
public:

	/// @brief ポートを開いてスマートフォンの接続を待機します。
	/// @param port ポート番号
	SIV3D_NODISCARD_CXX20
	SPInput(uint16 port);

	/// @brief 到着したデータを処理します。毎フレーム呼び出してください。
	void update();

	/// @brief ポートを閉じて再度開きます。
	void reset();

	/// @brief ポートを閉じて再度開きます。
	/// @param port ポート番号
	void reset(uint16 port);

	/// @brief 開いているポートの URL を返します。
	/// @return URL, ネットワークに接続されていない場合は空文字列
	[[nodiscard]]
	URL getURL() const;

	/// @brief スマートフォンとの接続が確立しているかを返します。
	/// @return スマートフォンとの接続が確立している場合 true, それ以外の場合は false
	[[nodiscard]]
	bool connected() const noexcept;

	/// @brief スマートフォンの画面サイズを返します。
	/// @return スマートフォンの画面サイズ
	[[nodiscard]]
	Size screenSize() const noexcept;

	/// @brief 現在のフレームでスマートフォンの画面がリサイズされたかを返します。
	/// @return 現在のフレームでスマートフォンの画面がリサイズされた場合 true, それ以外の場合は false
	[[nodiscard]]
	bool resized() const noexcept;

	/// @brief すべての接触点の情報を返します。
	/// @return すべての接触点の情報
	[[nodiscard]]
	const Array<SPTouch>& touches() const noexcept;

	/// @brief 新たに接触した接触点の identifier を返します。
	/// @return 新たに接触した接触点の identifier
	[[nodiscard]]
	const Array<int32>& startedTouches() const noexcept;

	/// @brief 移動した接触点の identifier を返します。
	/// @return 移動した接触点の identifier
	[[nodiscard]]
	const Array<int32>& movedTouches() const noexcept;

	/// @brief 離れた接触点の identifier を返します。
	/// @return 離れた接触点の identifier
	[[nodiscard]]
	const Array<int32>& endedTouches() const noexcept;

	/// @brief キャンセルされた接触点の identifier を返します。
	/// @return キャンセルされた接触点の identifier
	[[nodiscard]]
	const Array<int32>& canceledTouches() const noexcept;

private:

	uint16 m_port;

	bool m_connected = false;

	TCPServer m_server;

	Array<Byte> m_buffer;

	std::string m_startLine;
	std::string_view m_method;
	std::string_view m_resource;
	size_t m_contentSize = 0;

	std::string m_htmlResponseHeader;
	Blob m_htmlData;

	Size m_screenSize{};
	bool m_resized = false;

	Array<SPTouch> m_touches;
	Array<int32> m_startedTouches;
	Array<int32> m_movedTouches;
	Array<int32> m_endedTouches;
	Array<int32> m_canceledTouches;

	void sendString(std::string_view startLine);
	void sendHTML();
	void sendEmptyResponse(std::string_view status);
	void sendOK();
	void sendBadRequest();
	void sendNotFound();
	void sendTeapot();

	bool parseHeader();
	bool parseJSON(const JSON& data);
	bool sendResponse();
};
