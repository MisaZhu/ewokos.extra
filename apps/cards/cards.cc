#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Container.h>
#include <Widget/LabelButton.h>
#include <x++/X.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <ewoksys/kernel_tic.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace Ewok;

// 颜色定义 - Windows经典接龙风格
static const uint32_t COLOR_BG          = 0xFF008080; // 经典接龙桌布绿
static const uint32_t COLOR_CARD_BG     = 0xFFFFFFFF; // 卡牌背景白色
static const uint32_t COLOR_CARD_BACK   = 0xFF0000CC; // 卡牌背面蓝色
static const uint32_t COLOR_CARD_BACK_P = 0xFFFF0000; // 卡牌背面图案红色
static const uint32_t COLOR_CARD_BORDER = 0xFF000000; // 卡牌边框黑色
static const uint32_t COLOR_RED         = 0xFFCC0000; // 红色花色
static const uint32_t COLOR_BLACK       = 0xFF000000; // 黑色花色
static const uint32_t COLOR_SLOT        = 0xFF006666; // 牌槽颜色
static const uint32_t COLOR_SLOT_BORDER = 0xFF004D4D; // 牌槽边框
static const uint32_t COLOR_HIGHLIGHT    = 0xFFFFFF00; // 高亮黄色
static const uint32_t COLOR_BTN_BG      = 0xFFD4D0C8; // 按钮灰色
static const uint32_t COLOR_BTN_TEXT    = 0xFF000000; // 按钮文字黑色
static const uint32_t COLOR_TITLE_BAR  = 0xFF000080; // 标题栏深蓝
static const uint32_t COLOR_TITLE_TEXT  = 0xFFFFFFFF; // 标题白色

// 卡牌尺寸
static const int CARD_W = 56;
static const int CARD_H = 80;
static const int CARD_OFFSET_OPEN  = 22; // 翻开的牌叠偏移
static const int CARD_OFFSET_CLOSE = 8;  // 未翻开的牌叠偏移
static const int PILE_SPACING_X    = 12; // 列间距
static const int PILE_SPACING_Y    = 16; // 行间距
static const int HEADER_HEIGHT     = 28; // 顶部文字栏高度
static const int TOP_PADDING       = HEADER_HEIGHT + 8; // 顶部牌堆起始Y
static const int LEFT_PADDING      = 12;
static const int TOP_ROW_HEIGHT    = CARD_H + PILE_SPACING_Y;

// 花色
enum Suit {
    SUIT_HEARTS   = 0,
    SUIT_DIAMONDS = 1,
    SUIT_CLUBS    = 2,
    SUIT_SPADES   = 3
};

// 牌面值 1=A, 11=J, 12=Q, 13=K
typedef struct {
    int value;     // 1-13
    int suit;      // Suit
    bool faceUp;   // 是否翻开
} Card;

// 牌堆类型
enum PileType {
    PILE_STOCK     = 0, // 发牌堆
    PILE_WASTE     = 1, // 弃牌堆
    PILE_FOUNDATION = 2, // 终结堆 (4个)
    PILE_TABLEAU    = 3  // 桌面堆 (7个)
};

// 牌堆
typedef struct {
    PileType type;
    int x, y;       // 牌堆位置
    Card cards[24]; // 最多24张
    int count;
} Pile;

// 拖拽中的卡牌
typedef struct {
    Pile* srcPile;
    int srcIndex;          // 在源牌堆中的起始索引
    Card cards[13];        // 拖拽中的卡牌
    int count;
    int offsetX, offsetY; // 鼠标偏移
    int curX, curY;        // 当前位置
} DragInfo;

// 卡牌游戏主组件
class CardsWidget : public Widget {
private:
    Pile stock;       // 发牌堆
    Pile waste;       // 弃牌堆
    Pile foundation[4]; // 终结堆
    Pile tableau[7];    // 桌面堆

    DragInfo drag;
    bool dragging;

    int score;
    int moves;
    bool won;
    uint64_t winStartTime;
    uint32_t winFrame;

    // 获取花色字符
    const char* getSuitChar(int suit) {
        switch(suit) {
            case SUIT_HEARTS:   return "H";
            case SUIT_DIAMONDS: return "D";
            case SUIT_CLUBS:    return "C";
            case SUIT_SPADES:   return "S";
            default: return "?";
        }
    }

    // 获取牌面值字符
    const char* getValueChar(int value) {
        switch(value) {
            case 1:  return "A";
            case 2:  return "2";
            case 3:  return "3";
            case 4:  return "4";
            case 5:  return "5";
            case 6:  return "6";
            case 7:  return "7";
            case 8:  return "8";
            case 9:  return "9";
            case 10: return "10";
            case 11: return "J";
            case 12: return "Q";
            case 13: return "K";
            default: return "?";
        }
    }

    // 是否为红色花色
    bool isRed(int suit) {
        return suit == SUIT_HEARTS || suit == SUIT_DIAMONDS;
    }

    // 初始化牌堆位置
    void initPiles() {
        // 顶部行：发牌堆、弃牌堆、空、4个终结堆
        stock.type = PILE_STOCK;
        stock.x = LEFT_PADDING;
        stock.y = TOP_PADDING;
        stock.count = 0;

        waste.type = PILE_WASTE;
        waste.x = LEFT_PADDING + (CARD_W + PILE_SPACING_X);
        waste.y = TOP_PADDING;
        waste.count = 0;

        // 4个终结堆在右侧
        for(int i = 0; i < 4; i++) {
            foundation[i].type = PILE_FOUNDATION;
            foundation[i].x = LEFT_PADDING + (3 + i) * (CARD_W + PILE_SPACING_X);
            foundation[i].y = TOP_PADDING;
            foundation[i].count = 0;
        }

        // 7个桌面堆
        for(int i = 0; i < 7; i++) {
            tableau[i].type = PILE_TABLEAU;
            tableau[i].x = LEFT_PADDING + i * (CARD_W + PILE_SPACING_X);
            tableau[i].y = TOP_PADDING + TOP_ROW_HEIGHT;
            tableau[i].count = 0;
        }
    }

    // 创建一副牌并洗牌
    void createDeck() {
        Card deck[52];
        for(int s = 0; s < 4; s++) {
            for(int v = 1; v <= 13; v++) {
                int idx = s * 13 + (v - 1);
                deck[idx].value = v;
                deck[idx].suit = s;
                deck[idx].faceUp = false;
            }
        }

        // Fisher-Yates 洗牌
        for(int i = 51; i > 0; i--) {
            int j = rand() % (i + 1);
            Card tmp = deck[i];
            deck[i] = deck[j];
            deck[j] = tmp;
        }

        // 发到桌面堆：第i堆发i+1张，最后一张翻开
        int idx = 0;
        for(int col = 0; col < 7; col++) {
            for(int row = 0; row <= col; row++) {
                Card c = deck[idx++];
                if(row == col) c.faceUp = true;
                tableau[col].cards[tableau[col].count++] = c;
            }
        }

        // 剩余的发到发牌堆
        while(idx < 52) {
            Card c = deck[idx++];
            c.faceUp = false;
            stock.cards[stock.count++] = c;
        }
    }

    // 重新开始游戏
    void newGame() {
        initPiles();
        createDeck();
        score = 0;
        moves = 0;
        won = false;
        winStartTime = 0;
        winFrame = 0;
        dragging = false;
        memset(&drag, 0, sizeof(drag));
        update();
    }

    // 从发牌堆翻一张到弃牌堆
    void drawFromStock() {
        if(stock.count > 0) {
            Card c = stock.cards[--stock.count];
            c.faceUp = true;
            waste.cards[waste.count++] = c;
            moves++;
            update();
        }
        else if(waste.count > 0) {
            // 把弃牌堆的牌全部翻回发牌堆
            while(waste.count > 0) {
                Card c = waste.cards[--waste.count];
                c.faceUp = false;
                stock.cards[stock.count++] = c;
            }
            moves++;
            update();
        }
    }

    // 获取牌堆中第idx张牌的实际位置
    void getCardPos(Pile* p, int idx, int& cx, int& cy) {
        cx = p->x;
        cy = p->y;
        if(p->type == PILE_TABLEAU) {
            // 桌面堆：根据每张牌的状态计算偏移
            for(int i = 0; i < idx; i++) {
                if(p->cards[i].faceUp)
                    cy += CARD_OFFSET_OPEN;
                else
                    cy += CARD_OFFSET_CLOSE;
            }
        }
        else if(p->type == PILE_WASTE) {
            // 弃牌堆：最多显示3张展开
            int visible = p->count - idx;
            if(visible > 3) visible = 3;
            cx += (3 - visible) * 14;
        }
    }

    // 检查能否将卡牌放到目标牌堆
    bool canPlaceOn(Card c, Pile* dst) {
        if(dst->type == PILE_FOUNDATION) {
            // 终结堆：必须同花色，从A开始递增
            if(dst->count == 0)
                return c.value == 1;
            Card top = dst->cards[dst->count - 1];
            return top.suit == c.suit && top.value + 1 == c.value;
        }
        else if(dst->type == PILE_TABLEAU) {
            // 桌面堆：必须红黑交替，递减
            if(dst->count == 0)
                return c.value == 13; // 空堆只能放K
            Card top = dst->cards[dst->count - 1];
            if(!top.faceUp) return false;
            return isRed(top.suit) != isRed(c.suit) && top.value - 1 == c.value;
        }
        return false;
    }

    // 检查能否拖动一组牌（从srcPile的srcIndex开始）
    bool canDragFrom(Pile* srcPile, int srcIndex) {
        if(srcPile->type == PILE_STOCK) return false;
        if(srcPile->type == PILE_WASTE) {
            // 弃牌堆只能拖最上面一张
            return srcIndex == srcPile->count - 1;
        }
        if(srcPile->type == PILE_FOUNDATION) {
            // 终结堆只能拖最上面一张
            return srcIndex == srcPile->count - 1;
        }
        // 桌面堆：从srcIndex开始的所有牌必须翻开且形成有效序列
        if(srcPile->type == PILE_TABLEAU) {
            if(!srcPile->cards[srcIndex].faceUp) return false;
            for(int i = srcIndex; i < srcPile->count - 1; i++) {
                Card a = srcPile->cards[i];
                Card b = srcPile->cards[i + 1];
                if(!b.faceUp) return false;
                if(isRed(a.suit) == isRed(b.suit)) return false;
                if(a.value - 1 != b.value) return false;
            }
            return true;
        }
        return false;
    }

    // 查找鼠标点击的卡牌
    bool findCardAt(int x, int y, Pile*& outPile, int& outIndex) {
        // 从后往前查找（后绘制的在上层）
        // 先查桌面堆
        for(int i = 6; i >= 0; i--) {
            Pile* p = &tableau[i];
            for(int j = p->count - 1; j >= 0; j--) {
                int cx, cy;
                getCardPos(p, j, cx, cy);
                if(x >= cx && x < cx + CARD_W && y >= cy && y < cy + CARD_H) {
                    outPile = p;
                    outIndex = j;
                    return true;
                }
            }
            // 检查空牌堆位置
            if(p->count == 0) {
                if(x >= p->x && x < p->x + CARD_W && y >= p->y && y < p->y + CARD_H) {
                    outPile = p;
                    outIndex = -1;
                    return true;
                }
            }
        }
        // 弃牌堆
        if(waste.count > 0) {
            int cx, cy;
            getCardPos(&waste, waste.count - 1, cx, cy);
            if(x >= cx && x < cx + CARD_W && y >= cy && y < cy + CARD_H) {
                outPile = &waste;
                outIndex = waste.count - 1;
                return true;
            }
        }
        // 终结堆
        for(int i = 3; i >= 0; i--) {
            Pile* p = &foundation[i];
            if(p->count > 0) {
                int cx, cy;
                getCardPos(p, p->count - 1, cx, cy);
                if(x >= cx && x < cx + CARD_W && y >= cy && y < cy + CARD_H) {
                    outPile = p;
                    outIndex = p->count - 1;
                    return true;
                }
            }
            else {
                if(x >= p->x && x < p->x + CARD_W && y >= p->y && y < p->y + CARD_H) {
                    outPile = p;
                    outIndex = -1;
                    return true;
                }
            }
        }
        // 发牌堆
        if(x >= stock.x && x < stock.x + CARD_W && y >= stock.y && y < stock.y + CARD_H) {
            outPile = &stock;
            outIndex = stock.count - 1;
            return true;
        }
        return false;
    }

    // 检查是否获胜
    void checkWin() {
        int total = 0;
        for(int i = 0; i < 4; i++) total += foundation[i].count;
        if(total == 52 && !won) {
            won = true;
            score = 1000;
            winFrame = 0;
        }
    }

    // 自动放到终结堆
    bool tryAutoMoveToFoundation(Pile* srcPile, int srcIndex) {
        if(srcIndex != srcPile->count - 1) return false;
        Card c = srcPile->cards[srcIndex];
        for(int i = 0; i < 4; i++) {
            if(canPlaceOn(c, &foundation[i])) {
                srcPile->count--;
                foundation[i].cards[foundation[i].count++] = c;
                if(srcPile->type == PILE_TABLEAU && srcPile->count > 0) {
                    srcPile->cards[srcPile->count - 1].faceUp = true;
                }
                score += 10;
                moves++;
                checkWin();
                return true;
            }
        }
        return false;
    }

    // 双击事件：尝试自动放到终结堆
    void onDoubleClick(int x, int y) {
        Pile* p;
        int idx;
        if(findCardAt(x, y, p, idx) && idx >= 0) {
            if(tryAutoMoveToFoundation(p, idx)) {
                update();
            }
        }
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // 背景
        graph_fill_rect(g, r.x, r.y, r.w, r.h, COLOR_BG);

        // 绘制顶部信息栏（深蓝色背景，不被牌堆覆盖）
        graph_fill_rect(g, r.x, r.y, r.w, HEADER_HEIGHT, COLOR_TITLE_BAR);
        // 信息栏下边界线
        graph_line(g, r.x, r.y + HEADER_HEIGHT - 1, r.x + r.w, r.y + HEADER_HEIGHT - 1, 0xFF000000);

        // 绘制顶部分数文字
        char info[64];
        snprintf(info, sizeof(info), "Score: %d   Moves: %d", score, moves);
        uint32_t tw, th;
        font_text_size(info, theme->getFont(), theme->basic.fontSize, &tw, &th);
        graph_draw_text_font(g, r.x + 8, r.y + (HEADER_HEIGHT - th) / 2, info,
            theme->getFont(), theme->basic.fontSize, COLOR_TITLE_TEXT);

        // 绘制按钮提示
        const char* hint = "N:New  S:Stock  Dbl:Auto";
        uint32_t hw, hh;
        font_text_size(hint, theme->getFont(), theme->basic.fontSize, &hw, &hh);
        graph_draw_text_font(g, r.x + r.w - hw - 8, r.y + (HEADER_HEIGHT - hh) / 2, hint,
            theme->getFont(), theme->basic.fontSize, COLOR_TITLE_TEXT);

        // 绘制发牌堆
        drawPileSlot(g, stock.x, stock.y, stock.count == 0);
        drawPile(g, &stock, theme);

        // 绘制弃牌堆
        drawPileSlot(g, waste.x, waste.y, waste.count == 0);
        drawPile(g, &waste, theme);

        // 绘制终结堆
        for(int i = 0; i < 4; i++) {
            drawPileSlot(g, foundation[i].x, foundation[i].y, foundation[i].count == 0);
            // 显示花色提示图形
            if(foundation[i].count == 0) {
                uint32_t col = isRed(i) ? 0xFF884444 : 0xFF444444;
                drawSuit(g, foundation[i].x + CARD_W / 2, foundation[i].y + CARD_H / 2, 24, i, col);
            }
            drawPile(g, &foundation[i], theme);
        }

        // 绘制桌面堆
        for(int i = 0; i < 7; i++) {
            drawPileSlot(g, tableau[i].x, tableau[i].y, tableau[i].count == 0);
            drawPile(g, &tableau[i], theme);
        }

        // 绘制拖拽中的卡牌（最上层）
        if(dragging && drag.count > 0) {
            for(int i = 0; i < drag.count; i++) {
                int cx = drag.curX;
                int cy = drag.curY + i * CARD_OFFSET_OPEN;
                drawCard(g, cx, cy, drag.cards[i], theme, true);
            }
        }

        // 胜利动画
        if(won) {
            int phase = (int)winFrame;

            // 让终结堆的卡牌产生浮动效果
            for(int i = 0; i < 4; i++) {
                if(foundation[i].count > 0) {
                    int baseX, baseY;
                    getCardPos(&foundation[i], foundation[i].count - 1, baseX, baseY);
                    // 给每张牌一个基于时间的不同浮动偏移
                    int offsetY = ((phase + i * 3) % 7 - 3) * 2;
                    int offsetX = ((phase + i * 2) % 5 - 2) * 2;
                    // 重绘最上面的牌，让它们漂浮
                    Card c = foundation[i].cards[foundation[i].count - 1];
                    drawCard(g, baseX + offsetX, baseY + offsetY, c, theme, true);
                }
            }

            // 烟花效果：在随机位置画彩色花色图形
            for(int i = 0; i < 25; i++) {
                // 使用伪随机，基于 phase 和 i
                int seed = (int)(phase * 137 + i * 53) % 1000;
                int x = r.x + (seed % r.w);
                seed = (seed * 7 + i * 31) % 1000;
                int y = r.y + (seed % r.h);
                int sz = 8 + ((phase + i) % 14);
                int suit = (phase + i) % 4;
                uint32_t colors[4] = {0xFFCC0000, 0xFF00AA00, 0xFF0066FF, 0xFFFFAA00};
                drawSuit(g, x, y, sz, suit, colors[i % 4]);
            }

            // 闪烁的星星
            uint32_t starColor = (phase % 2 == 0) ? 0xFFFFFF00 : 0xFFFFFFFF;
            for(int i = 0; i < 15; i++) {
                int seed = (int)(phase * 31 + i * 17) % 1000;
                int x = r.x + (seed % r.w);
                seed = (seed * 11 + i * 23) % 1000;
                int y = r.y + (seed % r.h);
                graph_fill_circle(g, x, y, 2, starColor);
            }

            // 胜利文字框
            graph_fill_round(g, r.x + r.w/2 - 110, r.y + r.h/2 - 35, 220, 70, 14, 0xFF000080);
            graph_round(g, r.x + r.w/2 - 110, r.y + r.h/2 - 35, 220, 70, 14, 3, 0xFFFFFFFF);
            const char* win = "You Win!  Score: 1000";
            uint32_t ww, wh;
            font_text_size(win, theme->getFont(), theme->basic.fontSize + 2, &ww, &wh);
            graph_draw_text_font(g, r.x + r.w/2 - ww/2, r.y + r.h/2 - wh,
                win, theme->getFont(), theme->basic.fontSize + 2, 0xFFFFFFFF);
            const char* hint = "Press N for new game";
            uint32_t hw, hh;
            font_text_size(hint, theme->getFont(), theme->basic.fontSize, &hw, &hh);
            graph_draw_text_font(g, r.x + r.w/2 - hw/2, r.y + r.h/2 + 4,
                hint, theme->getFont(), theme->basic.fontSize, 0xFFFFFF00);
        }
    }

    // 绘制牌堆空槽
    void drawPileSlot(graph_t* g, int x, int y, bool empty) {
        if(empty) {
            graph_fill_round(g, x, y, CARD_W, CARD_H, 6, COLOR_SLOT);
            graph_round(g, x, y, CARD_W, CARD_H, 6, 2, COLOR_SLOT_BORDER);
        }
    }

    // 绘制牌堆
    void drawPile(graph_t* g, Pile* p, XTheme* theme) {
        for(int i = 0; i < p->count; i++) {
            int cx, cy;
            getCardPos(p, i, cx, cy);
            // 桌面堆只绘制最后一张需要全部显示，前面的部分被覆盖
            // 但为了简化，全部绘制（性能足够）
            drawCard(g, cx, cy, p->cards[i], theme, p->cards[i].faceUp);
        }
    }

    // 绘制花色图形：(cx, cy)为中心，size为总尺寸
    void drawSuit(graph_t* g, int cx, int cy, int size, int suit, uint32_t color) {
        int r = size / 4;       // 圆半径
        int halfH = size / 2;   // 半高
        int halfW = size / 2;   // 半宽
        
        switch(suit) {
            case SUIT_HEARTS: {  // 红心
                // 顶部两个圆
                graph_fill_circle(g, cx - r, cy - r, r + 1, color);
                graph_fill_circle(g, cx + r, cy - r, r + 1, color);
                // 底部倒三角：从最宽(2*size/3)到0
                int triH = halfH + r;
                for(int i = 0; i < triH; i++) {
                    int lineW = (triH - i) * halfW / triH;
                    if(lineW <= 0) continue;
                    graph_line(g, cx - lineW, cy + i - r,
                               cx + lineW, cy + i - r, color);
                }
                break;
            }
            case SUIT_DIAMONDS: { // 方块 - 菱形
                for(int i = -halfH + 1; i < halfH; i++) {
                    int lineW = halfW - abs(i) * halfW / halfH;
                    if(lineW <= 0) continue;
                    graph_line(g, cx - lineW, cy + i, cx + lineW, cy + i, color);
                }
                break;
            }
            case SUIT_CLUBS: {   // 梅花 - 三个圆+底部
                graph_fill_circle(g, cx, cy - r, r, color);
                graph_fill_circle(g, cx - r - 1, cy, r, color);
                graph_fill_circle(g, cx + r + 1, cy, r, color);
                // 底部矩形
                graph_fill_rect(g, cx - r/2, cy + r - 1, r, r + 2, color);
                break;
            }
            case SUIT_SPADES: {  // 黑桃 - 类似反红心
                // 底部两个圆
                graph_fill_circle(g, cx - r, cy + r, r + 1, color);
                graph_fill_circle(g, cx + r, cy + r, r + 1, color);
                // 顶部正三角：从最宽到0
                int triH = halfH + r;
                for(int i = 0; i < triH; i++) {
                    int lineW = (triH - i) * halfW / triH;
                    if(lineW <= 0) continue;
                    graph_line(g, cx - lineW, cy - i + r,
                               cx + lineW, cy - i + r, color);
                }
                // 底部小柄
                graph_fill_rect(g, cx - r/3, cy + r + r - 1, r * 2 / 3, r + 2, color);
                break;
            }
        }
    }

    // 绘制单张卡牌
    void drawCard(graph_t* g, int x, int y, Card c, XTheme* theme, bool faceUp) {
        (void)theme;
        if(faceUp) {
            // 卡牌背景
            graph_fill_round(g, x, y, CARD_W, CARD_H, 4, COLOR_CARD_BG);
            graph_round(g, x, y, CARD_W, CARD_H, 4, 1, COLOR_CARD_BORDER);

            // 卡牌颜色
            uint32_t col = isRed(c.suit) ? COLOR_RED : COLOR_BLACK;
            const char* vs = getValueChar(c.value);

            // 左上角：值 + 花色图形
            graph_draw_text_font(g, x + 3, y + 2, vs, theme->getFont(),
                theme->basic.fontSize, col);
            // 小尺寸花色图形
            drawSuit(g, x + 3 + 5, y + 2 + theme->basic.fontSize + 6,
                     10, c.suit, col);

            // 中心大花色图形
            drawSuit(g, x + CARD_W / 2, y + CARD_H / 2, 32, c.suit, col);

            // 右下角：值 + 花色图形（简化，仅绘制值和小图形）
            uint32_t vw, vh;
            font_text_size(vs, theme->getFont(), theme->basic.fontSize, &vw, &vh);
            graph_draw_text_font(g, x + CARD_W - vw - 3, y + CARD_H - vh - 2,
                vs, theme->getFont(), theme->basic.fontSize, col);
            drawSuit(g, x + CARD_W - 3 - 5, y + CARD_H - vh - 2 - 6,
                     10, c.suit, col);
        }
        else {
            // 卡牌背面 - 经典蓝色背底装饰图案
            // 外框
            graph_fill_round(g, x, y, CARD_W, CARD_H, 4, COLOR_CARD_BACK);
            // 内框
            graph_round(g, x, y, CARD_W, CARD_H, 4, 1, COLOR_CARD_BG);
            // 内部装饰：三个小矩形形成菱形
            graph_fill_round(g, x + 4, y + 4, CARD_W - 8, CARD_H - 8, 2, COLOR_CARD_BG);
            graph_round(g, x + 4, y + 4, CARD_W - 8, CARD_H - 8, 2, 1, COLOR_CARD_BG);
            // 红色外框
            graph_round(g, x, y, CARD_W, CARD_H, 4, 1, COLOR_CARD_BORDER);
            // 中心装饰 - 中间一个装饰的图案
            // 小的红色中心标记
            int cx = x + CARD_W / 2;
            int cy = y + CARD_H / 2;
            graph_fill_round(g, cx - 6, cy - 18, 12, 12, 2, COLOR_CARD_BACK_P);
            graph_fill_round(g, cx - 18, cy - 6, 12, 12, 2, COLOR_CARD_BACK_P);
            graph_fill_round(g, cx - 6, cy + 6, 12, 12, 2, COLOR_CARD_BACK_P);
            graph_fill_round(g, cx + 6, cy - 6, 12, 12, 2, COLOR_CARD_BACK_P);
            graph_fill_circle(g, cx, cy, 4, COLOR_CARD_BACK_P);
        }
    }

    bool onMouse(xevent_t* ev) {
        gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
        int x = pos.x;
        int y = pos.y;

        if(ev->state == MOUSE_STATE_DOWN) {
            // 检查是否点击在顶部信息栏区域（按钮）
            if(y < TOP_PADDING) {
                return true;
            }

            // 检查发牌堆点击
            if(x >= stock.x && x < stock.x + CARD_W &&
               y >= stock.y && y < stock.y + CARD_H) {
                drawFromStock();
                return true;
            }

            // 查找点击的卡牌
            Pile* p;
            int idx;
            if(findCardAt(x, y, p, idx) && idx >= 0) {
                if(canDragFrom(p, idx)) {
                    // 开始拖拽
                    dragging = true;
                    drag.srcPile = p;
                    drag.srcIndex = idx;
                    drag.count = 0;
                    for(int i = idx; i < p->count; i++) {
                        drag.cards[drag.count++] = p->cards[i];
                    }
                    int cardX, cardY;
                    getCardPos(p, idx, cardX, cardY);
                    drag.offsetX = x - cardX;
                    drag.offsetY = y - cardY;
                    drag.curX = cardX;
                    drag.curY = cardY;
                    // 从源牌堆移除
                    p->count = idx;
                    update();
                    return true;
                }
            }
        }
        else if(ev->state == MOUSE_STATE_DRAG) {
            if(dragging) {
                drag.curX = x - drag.offsetX;
                drag.curY = y - drag.offsetY;
                update();
                return true;
            }
        }
        else if(ev->state == MOUSE_STATE_UP) {
            if(dragging) {
                // 查找放置位置
                Pile* dstPile = NULL;
                int bestDist = 0x7FFFFFFF;
                Card c = drag.cards[0];

                // 检查终结堆
                for(int i = 0; i < 4; i++) {
                    int dx = foundation[i].x + CARD_W / 2 - (drag.curX + CARD_W / 2);
                    int dy = foundation[i].y + CARD_H / 2 - (drag.curY + CARD_H / 2);
                    int dist = dx * dx + dy * dy;
                    if(dist < bestDist && canPlaceOn(c, &foundation[i])) {
                        bestDist = dist;
                        dstPile = &foundation[i];
                    }
                }

                // 检查桌面堆
                for(int i = 0; i < 7; i++) {
                    int tx = tableau[i].x;
                    int ty = tableau[i].y;
                    if(tableau[i].count > 0) {
                        int cx, cy;
                        getCardPos(&tableau[i], tableau[i].count - 1, cx, cy);
                        tx = cx;
                        ty = cy;
                    }
                    int dx = tx + CARD_W / 2 - (drag.curX + CARD_W / 2);
                    int dy = ty + CARD_H / 2 - (drag.curY + CARD_H / 2);
                    int dist = dx * dx + dy * dy;
                    if(dist < bestDist && canPlaceOn(c, &tableau[i])) {
                        bestDist = dist;
                        dstPile = &tableau[i];
                    }
                }

                if(dstPile != NULL) {
                    // 放置卡牌
                    for(int i = 0; i < drag.count; i++) {
                        dstPile->cards[dstPile->count++] = drag.cards[i];
                    }
                    // 翻开源牌堆的下一张
                    if(drag.srcPile->type == PILE_TABLEAU && drag.srcPile->count > 0) {
                        drag.srcPile->cards[drag.srcPile->count - 1].faceUp = true;
                    }
                    if(dstPile->type == PILE_FOUNDATION) score += 10;
                    moves++;
                    checkWin();
                }
                else {
                    // 放回原牌堆
                    for(int i = 0; i < drag.count; i++) {
                        drag.srcPile->cards[drag.srcPile->count++] = drag.cards[i];
                    }
                }

                dragging = false;
                drag.count = 0;
                update();
                return true;
            }
        }
        else if(ev->state == MOUSE_STATE_DOUBLE_CLICK) {
            // 双击：尝试自动放到终结堆
            onDoubleClick(x, y);
            return true;
        }
        return false;
    }

    bool onIM(xevent_t* ev) {
        if(ev->state == XIM_STATE_PRESS) {
            int key = ev->value.im.key_code;
            // N 键：新游戏
            if(key == 'n' || key == 'N') {
                newGame();
                return true;
            }
            // S 键：发牌
            if(key == 's' || key == 'S') {
                drawFromStock();
                return true;
            }
        }
        return false;
    }

    // 定时器回调 - 用于胜利动画
    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        (void)timerFPS;
        (void)timerSteps;
        if(won) {
            winFrame++;
            update();
        }
    }

public:
    CardsWidget() : Widget() {
        srand((unsigned int)time(NULL));
        dragging = false;
        memset(&drag, 0, sizeof(drag));
        score = 0;
        moves = 0;
        won = false;
        winFrame = 0;
        newGame();
    }
};

class Cards : public WidgetWin {
public:
    Cards() : WidgetWin() {
        RootWidget* root = new RootWidget();
        setRoot(root);
        root->setType(Container::VERTICAL);

        CardsWidget* game = new CardsWidget();
        root->add(game);

        // 启动定时器（用于胜利动画）- 10 FPS
        setTimer(10);
    }
};

int main(int argc, char** argv) {
    X x;
    Cards win;

    grect_t wr;
    wr.x = 100;
    wr.y = 60;
    wr.w = LEFT_PADDING * 2 + 7 * CARD_W + 6 * PILE_SPACING_X;
    wr.h = TOP_PADDING + TOP_ROW_HEIGHT + 19 * CARD_OFFSET_OPEN + CARD_H + 20;

    win.open(&x, -1, wr, "Cards", XWIN_STYLE_NO_RESIZE);
    widgetXRun(&x, &win);
    return 0;
}
