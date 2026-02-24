#pragma once
/*
 * OpenWrt cfg80211 symbol compatibility map.
 *
 * rtl88x2eu uses legacy cfg80211 names:
 *   - cfg80211_mgmt_tx_status()
 *   - cfg80211_rx_mgmt_khz()
 *
 * Some OpenWrt/backports kernels export only these equivalent symbols:
 *   - cfg80211_mgmt_tx_status_ext()
 *   - cfg80211_rx_mgmt_ext()
 *
 * To avoid load-time undefined symbols across different OpenWrt trees, this
 * header remaps calls to local wrappers that resolve symbols at runtime:
 *   1) try legacy symbol
 *   2) fallback to _ext symbol
 */
#include <net/cfg80211.h>
#include <linux/module.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))

struct rtw_cfg80211_tx_status_compat {
	u64 cookie;
	u64 tx_tstamp;
	u64 ack_tstamp;
	const u8 *buf;
	size_t len;
	bool ack;
};

struct rtw_cfg80211_rx_info_compat {
	int freq;
	int sig_dbm;
	bool have_link_id;
	u8 link_id;
	const u8 *buf;
	size_t len;
	u32 flags;
	u64 rx_tstamp;
	u64 ack_tstamp;
};

static inline void rtw_openwrt_cfg80211_mgmt_tx_status(struct wireless_dev *wdev,
							u64 cookie, const u8 *buf,
							size_t len, bool ack, gfp_t gfp)
{
	typedef void (*rtw_legacy_fn_t)(struct wireless_dev *, u64, const u8 *,
					 size_t, bool, gfp_t);
	typedef void (*rtw_ext_fn_t)(struct wireless_dev *, void *, gfp_t);
	rtw_legacy_fn_t legacy_fn;
	rtw_ext_fn_t ext_fn;

	legacy_fn = (rtw_legacy_fn_t)__symbol_get("cfg80211_mgmt_tx_status");
	if (legacy_fn) {
		legacy_fn(wdev, cookie, buf, len, ack, gfp);
		symbol_put_addr((void *)legacy_fn);
		return;
	}

	ext_fn = (rtw_ext_fn_t)__symbol_get("cfg80211_mgmt_tx_status_ext");
	if (ext_fn) {
		struct rtw_cfg80211_tx_status_compat status = {
			.cookie = cookie,
			.buf = buf,
			.len = len,
			.ack = ack,
		};

		ext_fn(wdev, &status, gfp);
		symbol_put_addr((void *)ext_fn);
	}
}

static inline bool rtw_openwrt_cfg80211_rx_mgmt_khz(struct wireless_dev *wdev,
						     int freq, int sig_dbm,
						     const u8 *buf, size_t len,
						     u32 flags)
{
	typedef bool (*rtw_legacy_fn_t)(struct wireless_dev *, int, int,
					const u8 *, size_t, u32);
	typedef bool (*rtw_ext_fn_t)(struct wireless_dev *, void *);
	rtw_legacy_fn_t legacy_fn;
	rtw_ext_fn_t ext_fn;

	legacy_fn = (rtw_legacy_fn_t)__symbol_get("cfg80211_rx_mgmt_khz");
	if (legacy_fn) {
		bool ret = legacy_fn(wdev, freq, sig_dbm, buf, len, flags);

		symbol_put_addr((void *)legacy_fn);
		return ret;
	}

	ext_fn = (rtw_ext_fn_t)__symbol_get("cfg80211_rx_mgmt_ext");
	if (ext_fn) {
		struct rtw_cfg80211_rx_info_compat info = {
			.freq = freq,
			.sig_dbm = sig_dbm,
			.buf = buf,
			.len = len,
			.flags = flags,
		};
		bool ret = ext_fn(wdev, &info);

		symbol_put_addr((void *)ext_fn);
		return ret;
	}

	return false;
}

static inline bool rtw_openwrt_cfg80211_rx_mgmt(struct wireless_dev *wdev, int freq,
						 int sig_dbm, const u8 *buf,
						 size_t len, u32 flags)
{
	return rtw_openwrt_cfg80211_rx_mgmt_khz(
		wdev, MHZ_TO_KHZ(freq), sig_dbm, buf, len, flags);
}

#define cfg80211_mgmt_tx_status rtw_openwrt_cfg80211_mgmt_tx_status
#define cfg80211_rx_mgmt_khz rtw_openwrt_cfg80211_rx_mgmt_khz
#define cfg80211_rx_mgmt rtw_openwrt_cfg80211_rx_mgmt
#endif
