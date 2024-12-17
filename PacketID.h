#pragma once

enum class PACKET_ID : unsigned short
{
    DEV_ECHO = 1,

    // ·Î±×ÀÎ
    LOGIN_REQ = 201,
    LOGIN_RES = 202,

    ROOM_ENTER_REQ = 206,
    ROOM_ENTER_RES = 207,
    ROOM_NEW_USER_NTF = 208,
    ROOM_USER_LIST_NTF = 209,

    ROOM_LEAVE_REQ = 215,
    ROOM_LEAVE_RES = 216,
    ROOM_LEAVE_USER_NTF = 217,

    ROOM_CHAT_REQ = 221,
    ROOM_CHAT_RES = 222,
    ROOM_CHAT_NOTIFY = 223,
};