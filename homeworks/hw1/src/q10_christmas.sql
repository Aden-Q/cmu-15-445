WITH cte AS
(
    SELECT 'Order'.Id AS OrderId, Date(OrderDate)
    FROM 'Order' INNER JOIN Customer ON 'Order'.CustomerId = Customer.Id
    WHERE CompanyName = 'Queen Cozinha' AND Date(OrderDate) = '2014-12-25'
),
cte2 AS
(
    SELECT od.OrderId, ProductId, ProductName
    FROM (OrderDetail INNER JOIN cte ON OrderDetail.OrderId = cte.OrderId) AS od, Product
    WHERE od.ProductId = Product.Id
    ORDER BY ProductId
)

SELECT group_concat(ProductName) AS prod_list
FROM cte2;