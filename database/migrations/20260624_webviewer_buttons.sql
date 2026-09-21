/*
    WebViewer dynamic buttons
    Run this script on the filter/proxy database used by KMTGuard.
*/

IF OBJECT_ID('dbo._WebViewerButtons', 'U') IS NULL
BEGIN
    CREATE TABLE dbo._WebViewerButtons
    (
        ID              INT IDENTITY(1,1) NOT NULL CONSTRAINT PK_WebViewerButtons PRIMARY KEY,
        DisplayOrder    INT NOT NULL CONSTRAINT DF_WebViewerButtons_DisplayOrder DEFAULT(0),
        Name            NVARCHAR(64) NOT NULL,
        IconPath        VARCHAR(260) NOT NULL,
        Url             VARCHAR(512) NOT NULL,
        FrameWidth      INT NOT NULL CONSTRAINT DF_WebViewerButtons_FrameWidth DEFAULT(1000),
        FrameHeight     INT NOT NULL CONSTRAINT DF_WebViewerButtons_FrameHeight DEFAULT(650),
        IsEnabled       BIT NOT NULL CONSTRAINT DF_WebViewerButtons_IsEnabled DEFAULT(1),
        CreatedAtUtc    DATETIME2(0) NOT NULL CONSTRAINT DF_WebViewerButtons_CreatedAtUtc DEFAULT(SYSUTCDATETIME()),
        UpdatedAtUtc    DATETIME2(0) NULL
    );

    CREATE INDEX IX_WebViewerButtons_EnabledOrder
        ON dbo._WebViewerButtons(IsEnabled, DisplayOrder, ID)
        INCLUDE(Name, IconPath, Url, FrameWidth, FrameHeight);
END
GO

IF NOT EXISTS (SELECT 1 FROM dbo._WebViewerButtons)
BEGIN
    INSERT dbo._WebViewerButtons
        (DisplayOrder, Name, IconPath, Url, FrameWidth, FrameHeight, IsEnabled)
    VALUES
        (1, N'Website', 'clientlibrary\guides\kmt_web_viewer_1.ddj', 'https://example.com', 1000, 650, 1);
END
GO

UPDATE dbo._WebViewerButtons
SET IconPath = 'clientlibrary\guides\kmt_web_viewer_1.ddj',
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE IconPath IN
(
    'interface\ifcommon\com_mid_button.ddj',
    'icon\etc\location_1.ddj'
);
GO

/*
    Examples:

    -- Add button
    INSERT dbo._WebViewerButtons
        (DisplayOrder, Name, IconPath, Url, FrameWidth, FrameHeight, IsEnabled)
    VALUES
        (2, N'Discord', 'interface\ifcommon\com_mid_button.ddj', 'https://discord.gg/YOURCODE', 1000, 650, 1);

    -- Disable button
    UPDATE dbo._WebViewerButtons SET IsEnabled = 0, UpdatedAtUtc = SYSUTCDATETIME() WHERE ID = 1;

    -- Edit URL/frame
    UPDATE dbo._WebViewerButtons
    SET Url = 'https://your-site.com',
        FrameWidth = 1100,
        FrameHeight = 700,
        UpdatedAtUtc = SYSUTCDATETIME()
    WHERE ID = 1;
*/
