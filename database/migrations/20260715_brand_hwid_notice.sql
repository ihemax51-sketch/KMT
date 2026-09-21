IF OBJECT_ID(N'dbo.__Notices', N'U') IS NOT NULL
BEGIN
    IF EXISTS (SELECT 1 FROM dbo.__Notices WHERE Name = N'HWID_SUCCES')
    BEGIN
        UPDATE dbo.__Notices
           SET String = N'Hardware ID verified successfully by KMTGuard.'
         WHERE Name = N'HWID_SUCCES';
    END
    ELSE
    BEGIN
        INSERT INTO dbo.__Notices (Name, String)
        VALUES (N'HWID_SUCCES', N'Hardware ID verified successfully by KMTGuard.');
    END
END
