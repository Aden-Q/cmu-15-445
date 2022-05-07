WITH cte AS
(
    SELECT ProductName, CompanyName, ContactName, ProductId, ROW_NUMBER() OVER(PARTITION BY ProductId ORDER BY OrderDate) AS rn
    FROM 'Order', OrderDetail, Customer, Product
    WHERE 'Order'.Id = OrderDetail.OrderId AND 'Order'.CustomerId = Customer.Id AND OrderDetail.ProductId = Product.Id AND Product.Discontinued = 1
    ORDER BY ProductName
)
SELECT ProductName, CompanyName, ContactName
FROM cte
WHERE rn = 1;