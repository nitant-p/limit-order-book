enum class Side {Buy, Sell};

struct Order {
    int id;
    Side side;
    int price;
    int quantity;
};