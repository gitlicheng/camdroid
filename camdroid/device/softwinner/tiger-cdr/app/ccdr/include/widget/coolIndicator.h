
#ifndef EXT_COOLINDICATOR_H
#define EXT_COOLINDICATOR_H


#ifdef  __cplusplus
extern "C" {
#endif

#define CTRL_COOL_INDICATOR            ("CoolIndicator")

#define TYPE_BARITEM            1
#define TYPE_BMPITEM            2
#define TYPE_TEXTITEM           3

/** Structure of the cool indicator item info */
typedef struct _COOL_INDICATOR_ITEMINFO
{
    /**
     * Identifier of the item. When the user clicked the item, this control
     * will send a notification message to the parent window
     * with the notification code to be equal to this identifier.
     */
    int id;

    /**
     * Type of the item, can be one of the following values:
     * - TYPE_BARITEM
     *   The item is a separator (a vertical line).
     * - TYPE_BMPITEM
     *   The item is a bitmap.
     * - TYPE_TEXTITEM
     *   The item is text.
     */
    int ItemType;

    /**
     * Pointer to the bitmap of the item, valid when the type is TYPE_BMPITEM.
     */
    PBITMAP Bmp;

    /**
     * Hint text of the item, will be displayed in the tooltip window.
     */
    const char *ItemHint;

    /**
     * Caption of the item, valid when the type is TPYE_TEXTITEM.
     */
    const char *Caption;

	gal_pixel sliderColor;
    /** Additional data of the item */
    DWORD dwAddData;
} COOL_INDICATOR_ITEMINFO;

typedef COOL_INDICATOR_ITEMINFO *PCOOL_INDICATOR_ITEMINFO;

/**
 * \def CBS_BMP_CUSTOM
 * \brief The item bitmap has customized size.
 *
 * \note For the control with this style, you should pass
 *       the width and the height of the item bitmap by 
 *       the argument \a dwAddData of \a CreateWindowEx function.
 *
 * \code
 * int item_width = 20;
 * int item_height = 20;
 *
 * CreateWindowEx (CTRL_COOLBAR, ..., MAKELONG (item_width, item_height)));
 * \endcode
 */
#define CBS_BMP_CUSTOM          0x0002

/**
 * \def CBM_ADDITEM
 * \brief Adds a new item in a coolbar control.
 *
 * \code
 * CBM_ADDITEM
 * COOLBARITEMINFO *newIteminfo;
 *
 * wParam = 0;
 * lParam = (LPARAM)newIteminfo;
 * \endcode
 *
 * \param newIteminfo Pointer to the item info structure of the new item 
 *             to be added.
 *
 * \return Zero when success, otherwise less than 0;
 */
#define CBM_ADDITEM             0xFE00

/**
 * \def CBM_ENABLE
 * \brief Sets an item to be enabled or disabled.
 *
 * \code
 * CBM_ENABLE
 * int id;
 * BOOL enabled;
 *
 * wParam = (WPARAM)id;
 * lParam = (LPARAM)enabled;
 * \endcode
 *
 * \param id The identifier of the item to change.
 * \param enabled TRUE to enable the item, FALSE to disable the item.
 *
 * \return Zero when success, otherwise less than 0.
 */
#define CBM_ENABLE              0xFE01

#define CBM_SWITCH2NEXT			0xFE02

#define CBM_CLEARITEM			0xFE03

    /** @} end of mgext_ctrl_coolbar_msgs */

    /** @} end of mgext_ctrl_coolbar */

    /** @} end of controls */
#ifdef  __cplusplus
}
#endif

#endif /* EXT_COLINDICATOR_H */

