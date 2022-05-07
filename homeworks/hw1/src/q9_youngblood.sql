WITH cte AS
(
    SELECT Territory.Id AS TerritoryId, RegionDescription
    FROM  Territory INNER JOIN Region ON Territory.RegionId = Region.Id
),
cte2 AS
(
    SELECT RegionDescription, FirstName, LastName, BirthDate, ROW_NUMBER() OVER(PARTITION BY RegionDescription ORDER BY BirthDate DESC) rn
    FROM cte, (Employee INNER JOIN EmployeeTerritory ON Employee.Id = EmployeeTerritory.EmployeeId) AS et
    WHERE cte.TerritoryId = et.TerritoryId
)
SELECT RegionDescription, FirstName, LastName, BirthDate
FROM cte2
WHERE rn = 1;