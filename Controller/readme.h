#ifndef README_H
#define README_H


/* Status Packet from Device
 * - (0) TickCount
 * - (1) TYPE  : Status(0) / Cmd(1) / Reponse(2) / Emergency(3)
 * - (2) LAMP1 : Left(g0) / Warning(g1)(g2) / Right(g3)
 * - (3) LAMP2 : Low(0)/Front(1)/Tail(2)/Auto(3) - Auto(4)/Tail(5)/Front(6)/Low(7)
 * - (4) TEMP  : 0~100
 * - (5) BTR   : 0~100
 * - (6) ILL   : 0~100
 * - (7) RESRV : 8 Bit
 */

/* Command to Device
 * (0) TickCount
 * (1) Status(0) / Cmd(1) / Reponse(2) / Emergency(3)
 * (2) TYPES
 *     0x01 TP_START
 *     0x02 TP_STOP
 *     0x10 TP_LAMP_OFF
 *     0x11 TP_LAMP_TAIL
 *     0x12 TP_LAMP_FRONT
 *     0x13 TP_LAMP_LOW
 *     0x20 TP_DIR_OFF
 *     0x21 TP_DIR_LEFT
 *     0x22 TP_DIR_RIGHT
 *     0x30 TP_WARNING
 *     0x0a TP_ACC
 *     0x0b TP_BRAKE
 * (3)
 *
 *
 */


#endif // README_H
