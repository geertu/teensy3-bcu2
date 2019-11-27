//
// USB and Serial Event Wrapper
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#ifdef __cplusplus
extern "C" {
#endif
	void usb_serial_event(void);
	void usb_serial2_event(void);
	void usb_serial3_event(void);
	void serial_event(void);
	void serial2_event(void);
#ifdef __cplusplus
}
#endif
