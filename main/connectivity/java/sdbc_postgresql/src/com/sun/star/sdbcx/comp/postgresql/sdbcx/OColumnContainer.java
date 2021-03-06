/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/

package com.sun.star.sdbcx.comp.postgresql.sdbcx;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.sun.star.beans.XPropertySet;
import com.sun.star.container.ElementExistException;
import com.sun.star.container.XNameAccess;
import com.sun.star.sdbc.ColumnValue;
import com.sun.star.sdbc.DataType;
import com.sun.star.sdbc.SQLException;
import com.sun.star.sdbc.XDatabaseMetaData;
import com.sun.star.sdbc.XStatement;
import com.sun.star.sdbcx.comp.postgresql.comphelper.CompHelper;
import com.sun.star.sdbcx.comp.postgresql.sdbcx.SqlTableHelper.ColumnDescription;
import com.sun.star.sdbcx.comp.postgresql.sdbcx.descriptors.SdbcxColumnDescriptor;
import com.sun.star.sdbcx.comp.postgresql.util.ComposeRule;
import com.sun.star.sdbcx.comp.postgresql.util.DbTools;
import com.sun.star.sdbcx.comp.postgresql.util.Osl;
import com.sun.star.uno.UnoRuntime;

public class OColumnContainer extends OContainer {
    private OTable table;
    private XDatabaseMetaData metadata;
    private Map<String,ColumnDescription> columnDescriptions = new HashMap<>();
    private Map<String,ExtraColumnInfo> extraColumnInfo = new HashMap<>();
    
    /// The XDatabaseMetaData.getColumns() data stored in columnDescriptions doesn't provide everything we need, so this class stores the rest.
    public static class ExtraColumnInfo {
        public boolean isAutoIncrement;
        public boolean isCurrency;
        public int dataType;
    }
    
    public OColumnContainer(Object lock, boolean isCaseSensitive, List<ColumnDescription> columnDescriptions, OTable table, XDatabaseMetaData metadata)
            throws ElementExistException {
        super(lock, isCaseSensitive, toColumnNames(columnDescriptions));
        this.table = table;
        this.metadata = metadata;
        for (ColumnDescription columnDescription : columnDescriptions) {
            this.columnDescriptions.put(columnDescription.columnName, columnDescription);
        }
    }
    
    private static List<String> toColumnNames(List<ColumnDescription> columns) {
        List<String> columnNames = new ArrayList<>(columns.size());
        for (ColumnDescription columnDescription : columns) {
            columnNames.add(columnDescription.columnName);
        }
        return columnNames;
    }
    
    @Override
    protected XPropertySet createObject(String name) throws SQLException {
        boolean queryInfo = true;
        boolean isAutoIncrement = false;
        boolean isCurrency = false;
        int dataType = DataType.OTHER;
        
        ColumnDescription columnDescription = columnDescriptions.get(name);
        if (columnDescription == null) {
            // could be a recently added column. Refresh:
            List<ColumnDescription> newColumns = new SqlTableHelper().readColumns(metadata, table.catalogName, table.schemaName, table.getName());
            for (ColumnDescription newColumnDescription : newColumns) {
                if (newColumnDescription.columnName.equals(name)) {
                    columnDescriptions.put(name, newColumnDescription);
                    break;
                }
            }
            columnDescription = columnDescriptions.get(name);
        }
        if (columnDescription == null) {
            throw new SQLException("No column " + name + " found");
        }
        
        ExtraColumnInfo columnInfo = extraColumnInfo.get(name);
        if (columnInfo == null) {
            String composedName = DbTools.composeTableNameForSelect(metadata.getConnection(), table);
            extraColumnInfo = DbTools.collectColumnInformation(metadata.getConnection(), composedName, "*");
            columnInfo = extraColumnInfo.get(name);
        }
        if (columnInfo != null) {
            queryInfo = false;
            isAutoIncrement = columnInfo.isAutoIncrement;
            isCurrency = columnInfo.isCurrency;
            dataType = columnInfo.dataType;
        }
        
        XNameAccess primaryKeyColumns = DbTools.getPrimaryKeyColumns(UnoRuntime.queryInterface(XPropertySet.class, table));
        int nullable = columnDescription.nullable;
        if (nullable != ColumnValue.NO_NULLS && primaryKeyColumns != null && primaryKeyColumns.hasByName(name)) {
            nullable = ColumnValue.NO_NULLS;
        }
        return new OColumn(name, columnDescription.typeName, columnDescription.defaultValue, columnDescription.remarks,
                nullable, columnDescription.columnSize, columnDescription.decimalDigits, columnDescription.type,
                isAutoIncrement, false, isCurrency, isCaseSensitive());
    }
    
    @Override
    protected XPropertySet createDescriptor() {
        return new SdbcxColumnDescriptor(isCaseSensitive());
    }
    
    @Override
    protected void impl_refresh() {
        extraColumnInfo.clear();
        // FIXME: won't help
        table.refreshColumns();
    }
    
    @Override
    protected XPropertySet appendObject(String _rForName, XPropertySet descriptor) throws SQLException {
        if (table == null) {
            return cloneDescriptor(descriptor);
        }
        String sql = String.format("ALTER TABLE %s ADD %s",
                DbTools.composeTableName(metadata, table, ComposeRule.InTableDefinitions, false, false, true),
                DbTools.createStandardColumnPart(descriptor, table.getConnection(), null, table.getTypeCreatePattern()));
        XStatement statement = null;
        try {
            statement = table.getConnection().createStatement();
            statement.execute(sql);
        } finally {
            CompHelper.disposeComponent(statement);
        }
        return createObject(_rForName);
    }
    
    @Override
    protected void dropObject(int index, String name) throws SQLException {
        Osl.ensure(table, "Table is null!");
        if (table == null) {
            return;
        }
        String quote = metadata.getIdentifierQuoteString();
        String sql = String.format("ALTER TABLE %s DROP %s",
                DbTools.composeTableName(metadata, table, ComposeRule.InTableDefinitions, false, false, true),
                DbTools.quoteName(quote, name));
        XStatement statement = null;
        try {
            statement = table.getConnection().createStatement();
            statement.execute(sql);
        } finally {
            CompHelper.disposeComponent(statement);
        }
    }
}
